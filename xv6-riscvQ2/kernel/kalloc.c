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
  int num_user_pages;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.num_user_pages = 0;
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
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
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Add function to check if we can allocate user page
int
can_alloc_user_page(void)
{
  acquire(&kmem.lock);
  int can = (kmem.num_user_pages < MAXUSERVMPAGES);
  release(&kmem.lock);
  return can;
}

// Add function to increment user page count
void
inc_user_pages(void)
{
  acquire(&kmem.lock);
  kmem.num_user_pages++;
  release(&kmem.lock);
}

// Add function to decrement user page count
void
dec_user_pages(void)
{
  acquire(&kmem.lock);
  if(kmem.num_user_pages > 0)
    kmem.num_user_pages--;
  release(&kmem.lock);
}

// Get current user page count
int
get_user_page_count(void)
{
  acquire(&kmem.lock);
  int count = kmem.num_user_pages;
  release(&kmem.lock);
  return count;
}
