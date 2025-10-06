// kernel/mru.h
#ifndef MRU_H
#define MRU_H

#include "types.h"

struct mru_entry {
  int pid;
  uint64 va;
  uint64 pa;
  struct mru_entry *next;
  struct mru_entry *prev;
};

void            mruinit(void);
void            mruadd(int pid, uint64 va, uint64 pa);
void            mruremove(uint64 pa);
void            mruupdate(int pid, uint64 va);
struct mru_entry* mruevict(void);
void            mrudump(void);
void            mrufree(int pid);

#endif