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


//used for the page ref cnt;
#define IND(pa) (((uint64)(pa)-KERNBASE)/PGSIZE)
struct ref {
  struct spinlock lock;
  int cnt[(PHYSTOP-KERNBASE)/PGSIZE];
} rec;

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&rec.lock, "ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    rec.cnt[IND((uint64)p)]=1;
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

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  if(decre(pa)==0){
    memset(pa, 1, PGSIZE);
    r = (struct run*)pa;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.

int incre(void* pa){
  int tem=0;
  acquire(&rec.lock);
  tem=++rec.cnt[IND(pa)];
  release(&rec.lock);
  return tem;
}
int decre(void* pa){
  int tem=0;
  acquire(&rec.lock);
  if((tem=--rec.cnt[IND(pa)])<0)
    panic("decre: negetive ref");
  release(&rec.lock);
  return tem;
}

void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    if(incre((void*)r)!=1)
      panic("kalloc: wrong alloc");
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}
