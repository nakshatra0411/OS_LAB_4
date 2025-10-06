// kernel/vm_ext.c (new)
#include "mru.h"
#include "proc.h"

void register_user_page(struct proc *p, uint64 va, char *kaddr) {
  struct mru_entry *e = kalloc(); // small struct allocated similarly to kmalloc or use static pool
  if(!e) return; // OOM
  e->pid = p->pid;
  e->vpn = va >> PGSHIFT;
  e->kaddr = kaddr;
  mru_add(e);
  // you should also store pointer to 'e' so later removal is O(1)
}
