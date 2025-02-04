// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int cnt[PHYSTOP / PGSIZE];
}cow_lock;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&cow_lock.lock,"cow_lock");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    cow_lock.cnt[(uint64) p / PGSIZE] = 1;
    kfree(p);
  }
    
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  //保证地址合法的逻辑
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  r = (struct run*)pa;

  acquire(&cow_lock.lock);
  if(--cow_lock.cnt[(uint64)pa / PGSIZE] == 0){//没有计数了才真正删掉
    release(&cow_lock.lock);

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);

  }else{
    release(&cow_lock.lock);
  }
  
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&cow_lock.lock);
    cow_lock.cnt[(uint64)r / PGSIZE] = 1;//初始化为1
    release(&cow_lock.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int kcowcntadd(void* pa){
  //保证地址合法
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return -1;

  acquire(&cow_lock.lock);
  cow_lock.cnt[(uint64)pa / PGSIZE]++;
  release(&cow_lock.lock);
  return 0;
}

int kiscow(pagetable_t pagetable, uint64 va){//检查一个页面是不是有COW标记
  if(va >= MAXVA)////保证地址合法,否则会walk panic
    return -1;
  pte_t* pte = walk(pagetable, va, 0);
  if(pte == 0)
    return -1;
  if(*pte & PTE_COW)return 0;
  else return -1;
}

void* kcowalloc(pagetable_t p, uint64 va){//真正分配内存
  va = PGROUNDDOWN(va);//!!!!必须页面对齐
  pte_t* pte;
  if((pte = walk(p, va, 0)) == 0)
    return 0;
  
  uint64 pa = PTE2PA(*pte);
  acquire(&cow_lock.lock);
  char* mem;
  if(cow_lock.cnt[pa / PGSIZE] == 1){//如果标记值为一就禁止COW位
    release(&cow_lock.lock);
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
    return (void*)pa;
  }else{
    cow_lock.cnt[pa / PGSIZE] --;//计数要减1
    release(&cow_lock.lock);
    //参考uvmcopy
    if((mem = kalloc()) == 0)//真正分配
      return 0;
    memmove(mem, (char*)pa, PGSIZE);

    *pte &= ~PTE_V;//防止remap！！
  
    //建立映射
    if(mappages(p,va , PGSIZE, (uint64)mem, (PTE_FLAGS(*pte) | PTE_W) & ~PTE_COW) != 0){
      kfree(mem);
      *pte |= PTE_V;
      return 0;
    }
    return mem;
  }

}

