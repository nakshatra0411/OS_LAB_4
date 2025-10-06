// kernel/mru.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "mru.h"

struct {
  struct spinlock lock;
  struct mru_entry *head;  // Most recently used
  struct mru_entry *tail;  // Least recently used
} mru_list;

#define MAX_MRU_ENTRIES 128
struct mru_entry entries[MAX_MRU_ENTRIES];
int entry_count = 0;

void
mruinit(void)
{
  initlock(&mru_list.lock, "mru");
  mru_list.head = 0;
  mru_list.tail = 0;
  entry_count = 0;
}

// Add a page to the MRU list (makes it most recently used)
void
mruadd(int pid, uint64 va, uint64 pa)
{
  acquire(&mru_list.lock);
  
  if(entry_count >= MAX_MRU_ENTRIES) {
    release(&mru_list.lock);
    return;
  }
  
  struct mru_entry *e = &entries[entry_count++];
  e->pid = pid;
  e->va = va;
  e->pa = pa;
  e->next = mru_list.head;
  e->prev = 0;
  
  if(mru_list.head)
    mru_list.head->prev = e;
  mru_list.head = e;
  
  if(mru_list.tail == 0)
    mru_list.tail = e;
  
  release(&mru_list.lock);
}

// Remove a page from the MRU list by physical address
void
mruremove(uint64 pa)
{
  acquire(&mru_list.lock);
  
  struct mru_entry *e = mru_list.head;
  while(e) {
    if(e->pa == pa) {
      if(e->prev)
        e->prev->next = e->next;
      else
        mru_list.head = e->next;
      
      if(e->next)
        e->next->prev = e->prev;
      else
        mru_list.tail = e->prev;
      
      break;
    }
    e = e->next;
  }
  
  release(&mru_list.lock);
}

// Update MRU position (move to head when accessed)
void
mruupdate(int pid, uint64 va)
{
  acquire(&mru_list.lock);
  
  struct mru_entry *e = mru_list.head;
  while(e) {
    if(e->pid == pid && e->va == va) {
      // Already at head
      if(e == mru_list.head) {
        release(&mru_list.lock);
        return;
      }
      
      // Remove from current position
      if(e->prev)
        e->prev->next = e->next;
      if(e->next)
        e->next->prev = e->prev;
      else
        mru_list.tail = e->prev;
      
      // Move to head
      e->next = mru_list.head;
      e->prev = 0;
      if(mru_list.head)
        mru_list.head->prev = e;
      mru_list.head = e;
      
      release(&mru_list.lock);
      return;
    }
    e = e->next;
  }
  
  release(&mru_list.lock);
}

// Evict the MRU page (head of list)
struct mru_entry*
mruevict(void)
{
  acquire(&mru_list.lock);
  
  struct mru_entry *victim = mru_list.head;
  
  if(victim == 0) {
    release(&mru_list.lock);
    return 0;
  }
  
  // Remove from head
  mru_list.head = victim->next;
  if(mru_list.head)
    mru_list.head->prev = 0;
  else
    mru_list.tail = 0;
  
  release(&mru_list.lock);
  
  return victim;
}

// Dump MRU list to console
void
mrudump(void)
{
  acquire(&mru_list.lock);
  
  printf("MRU List (Most -> Least Recently Used):\n");
  struct mru_entry *e = mru_list.head;
  int count = 0;
  while(e) {
    printf("  [%d] PID=%d VA=0x%lx PA=0x%lx\n", count++, e->pid, e->va, e->pa);
    e = e->next;
  }
  if(count == 0)
    printf("  (empty)\n");
  
  release(&mru_list.lock);
}

// Remove all entries for a process
void
mrufree(int pid)
{
  acquire(&mru_list.lock);
  
  struct mru_entry *e = mru_list.head;
  while(e) {
    struct mru_entry *next = e->next;
    if(e->pid == pid) {
      if(e->prev)
        e->prev->next = e->next;
      else
        mru_list.head = e->next;
      
      if(e->next)
        e->next->prev = e->prev;
      else
        mru_list.tail = e->prev;
    }
    e = next;
  }
  
  release(&mru_list.lock);
}