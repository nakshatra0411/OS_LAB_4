// kernel/swap.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "swap.h"

struct {
  struct spinlock lock;
  struct swapslot slots[SWAPBLOCKS];
  char data[SWAPBLOCKS][PGSIZE];  // Actually store the swapped data
} swaptable;

void
swapinit(void)
{
  initlock(&swaptable.lock, "swaptable");
  for(int i = 0; i < SWAPBLOCKS; i++) {
    swaptable.slots[i].used = 0;
    swaptable.slots[i].pid = 0;
    swaptable.slots[i].va = 0;
  }
  printf("swapinit: initialized %d swap blocks\n", SWAPBLOCKS);
}

// Find a free swap slot and write page to it
int
swapout(int pid, uint64 va, char* page)
{
  acquire(&swaptable.lock);
  
  // Find free slot
  int slot = -1;
  for(int i = 0; i < SWAPBLOCKS; i++) {
    if(swaptable.slots[i].used == 0) {
      slot = i;
      break;
    }
  }
  
  if(slot < 0) {
    release(&swaptable.lock);
    printf("swapout: no free swap space\n");
    return -1;
  }
  
  // Mark slot as used and copy data
  swaptable.slots[slot].used = 1;
  swaptable.slots[slot].pid = pid;
  swaptable.slots[slot].va = va;
  memmove(swaptable.data[slot], page, PGSIZE);
  
  release(&swaptable.lock);
  
  printf("swapout: PID=%d VA=0x%lx -> slot %d\n", pid, va, slot);
  
  return slot;
}

// Read page from swap slot
int
swapin(int pid, uint64 va, char* page)
{
  acquire(&swaptable.lock);
  
  // Find the swap slot
  int slot = -1;
  for(int i = 0; i < SWAPBLOCKS; i++) {
    if(swaptable.slots[i].used && 
       swaptable.slots[i].pid == pid && 
       swaptable.slots[i].va == va) {
      slot = i;
      break;
    }
  }
  
  if(slot < 0) {
    release(&swaptable.lock);
    printf("swapin: not found PID=%d VA=0x%lx\n", pid, va);
    return -1;
  }
  
  // Copy data back and free the swap slot
  memmove(page, swaptable.data[slot], PGSIZE);
  swaptable.slots[slot].used = 0;
  swaptable.slots[slot].pid = 0;
  swaptable.slots[slot].va = 0;
  
  release(&swaptable.lock);
  
  printf("swapin: PID=%d VA=0x%lx from slot %d\n", pid, va, slot);
  
  return 0;
}

// Free all swap slots for a process
void
swapfree(int pid)
{
  acquire(&swaptable.lock);
  
  int count = 0;
  for(int i = 0; i < SWAPBLOCKS; i++) {
    if(swaptable.slots[i].used && swaptable.slots[i].pid == pid) {
      swaptable.slots[i].used = 0;
      swaptable.slots[i].pid = 0;
      swaptable.slots[i].va = 0;
      count++;
    }
  }
  
  release(&swaptable.lock);
  
  if(count > 0)
    printf("swapfree: freed %d slots for PID=%d\n", count, pid);
}