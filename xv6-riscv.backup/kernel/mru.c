// kernel/mru.c
#include "types.h"
#include "defs.h"
#include "spinlock.h"
#include "mru.h"

struct {
  struct spinlock lock;
  struct mru_entry *head; // head = most recently used
  struct mru_entry *tail; // tail = least recently used
} mru;

void mru_init(void) {
  initlock(&mru.lock, "mru");
  mru.head = mru.tail = 0;
}

static void link_head(struct mru_entry *e) {
  e->prev = 0;
  e->next = mru.head;
  if(mru.head) mru.head->prev = e;
  mru.head = e;
  if(!mru.tail) mru.tail = e;
}

static void unlink(struct mru_entry *e) {
  if(e->prev) e->prev->next = e->next;
  else mru.head = e->next;
  if(e->next) e->next->prev = e->prev;
  else mru.tail = e->prev;
  e->prev = e->next = 0;
}

void mru_touch(int pid, uint64 va) {
  acquire(&mru.lock);
  // find the entry: linear search or use a hash map for speed.
  // For simplicity here, walk the list:
  for(struct mru_entry *e = mru.head; e; e = e->next) {
    if(e->pid == pid && e->vpn == (va >> PGSHIFT)) {
      unlink(e);
      link_head(e);
      release(&mru.lock);
      return;
    }
  }
  // Not found -> nothing (maybe a new allocated page). The caller can add.
  release(&mru.lock);
}

void mru_add(struct mru_entry *e) {
  acquire(&mru.lock);
  link_head(e);
  release(&mru.lock);
}

void mru_remove(struct mru_entry *e) {
  acquire(&mru.lock);
  unlink(e);
  release(&mru.lock);
}

struct mru_entry* mru_evict(void) {
  acquire(&mru.lock);
  struct mru_entry *victim = mru.head; // MRU = head
  if(victim) unlink(victim);
  release(&mru.lock);
  return victim;
}

// optionally a debug dump:
void mru_dump(void) {
  acquire(&mru.lock);
  for(struct mru_entry *e = mru.head; e; e = e->next) {
    cprintf("MRU: pid %d vpn 0x%x\n", e->pid, (uint)e->vpn);
  }
  release(&mru.lock);
}
