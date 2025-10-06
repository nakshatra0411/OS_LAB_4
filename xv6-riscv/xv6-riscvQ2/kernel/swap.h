// kernel/swap.h
#ifndef SWAP_H
#define SWAP_H

#include "types.h"
#include "param.h"

#define SWAPBLOCKS 1024  // Number of swap blocks
#define SWAPSIZE (SWAPBLOCKS * PGSIZE)

struct swapslot {
  int used;
  int pid;
  uint64 va;  // Virtual address of the swapped page
};

void            swapinit(void);
int             swapout(int pid, uint64 va, char* page);
int             swapin(int pid, uint64 va, char* page);
void            swapfree(int pid);

#endif