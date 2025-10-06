// in kernel/swap.c
#include "types.h"
#include "defs.h"
#include "fs.h"
#include "file.h"
#include "proc.h"

static int ensure_swapfile(struct proc *p) {
  if(p->swapfile_fd >= 0) return 0;
  // create file /swap/<pid>
  char name[64];
  safestrcpy(name, "/swap/", sizeof(name));
  // building names in xv6/fs is limited; simpler approach:
  // Use creat(filename) (if xv6 has sysfile or filecreate internals).
  // For simplicity in project: you can create a special file using file allocation
  // routines or reuse an existing per-proc file handle stored in proc.
  // Implementation detail depends on your xv6 fs support.
  return 0;
}

int swap_write(struct proc *p, uint64 vpn, void *kaddr) {
  ensure_swapfile(p);
  int slot = assign_slot_for_vpn(p, vpn); // implement mapping
  // write at offset slot * PGSIZE
  filewrite(p->swapfile, kaddr, PGSIZE, slot * PGSIZE);
  return slot;
}

int swap_read(struct proc *p, uint64 vpn, void *kaddr) {
  int slot = p->swap_slot[vpn];
  if(slot < 0) return -1;
  fileread(p->swapfile, kaddr, PGSIZE, slot * PGSIZE);
  p->swap_slot[vpn] = -1; // free slot (or mark)
  return 0;
}
