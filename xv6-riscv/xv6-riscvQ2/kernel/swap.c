// kernel/swap.c  -- per-process swap file implementation
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "stat.h"
#include "fcntl.h"
#include "fs.h"
#include "sleeplock.h"

// No global large data store — swap is on per-process file.
// swapinit keeps nothing but initializes any required locks (none needed now).
void
swapinit(void)
{
  // nothing global to init (keep function for compatibility)
  // optionally print a message
  printf("swapinit: per-process swap enabled\n");
}

// Helper: generate swap filename for pid
static void
swap_fname_for_pid(int pid, char *buf, int len)
{
  // ensure length is enough: "swapfile-%d" fits in 32
  safestrcpy(buf, "swapfile-", len);
  int off = strlen(buf);
  // append pid
  char tmp[16];
  int n = snprintf(tmp, sizeof(tmp), "%d", pid);
  if(n >= sizeof(tmp)) n = sizeof(tmp)-1;
  memmove(buf + off, tmp, n);
  buf[off + n] = '\0';
}

// Create swap file for process p (if not already created)
static int
ensure_swapfile(struct proc *p)
{
  if(p->swapip != 0)
    return 0; // already exists

  char fname[32];
  swap_fname_for_pid(p->pid, fname, sizeof(fname));
  begin_op();
  struct inode *ip = create(fname, T_FILE, 0, 0);
  if(ip == 0){
    end_op();
    cprintf("ensure_swapfile: create failed for %s\n", fname);
    return -1;
  }
  iunlock(ip); // create returns locked; unlock so we can use readi/writei safely
  end_op();

  p->swapip = ip;
  p->next_swap_slot = 0;
  p->num_swapped = 0;
  // zero the swaptable entries
  for(int i = 0; i < MAX_SWAP_SLOTS; i++){
    p->swaptable[i].used = 0;
    p->swaptable[i].va = 0;
    p->swaptable[i].slotno = -1;
  }
  return 0;
}

// Write one page to this process swap file.  Returns slot number or -1.
int
swapout(int pid, uint64 va, char *page)
{
  struct proc *p = findproc(pid);
  if(p == 0){
    cprintf("swapout: pid %d not found\n", pid);
    return -1;
  }

  // ensure file exists
  if(ensure_swapfile(p) < 0)
    return -1;

  // find a free slot number
  int slot = -1;
  // first try simple next_slot approach
  for(int i = 0; i < MAX_SWAP_SLOTS; i++){
    int s = (p->next_swap_slot + i) % MAX_SWAP_SLOTS;
    if(p->swaptable[s].used == 0){
      slot = s;
      break;
    }
  }
  if(slot < 0){
    cprintf("swapout: no free per-process swap slot for pid %d\n", pid);
    return -1;
  }

  // write page at offset = slot * PGSIZE
  uint off = (uint)slot * PGSIZE;

  begin_op();
  // writei returns number of bytes written or <0 on error
  int wrote = writei(p->swapip, page, off, PGSIZE);
  if(wrote != PGSIZE){
    end_op();
    cprintf("swapout: writei failed for pid %d slot %d wrote %d\n", pid, slot, wrote);
    return -1;
  }
  end_op();

  // update p->swaptable
  p->swaptable[slot].used = 1;
  p->swaptable[slot].va = va;
  p->swaptable[slot].slotno = slot;
  p->num_swapped++;
  p->next_swap_slot = (slot + 1) % MAX_SWAP_SLOTS;

  // Debug print
  cprintf("swapout: pid %d va 0x%lx -> slot %d\n", pid, va, slot);
  return slot;
}

// Read page back from per-process swap file. returns 0 or -1
int
swapin(int pid, uint64 va, char *page)
{
  struct proc *p = findproc(pid);
  if(p == 0){
    cprintf("swapin: pid %d not found\n", pid);
    return -1;
  }
  if(p->swapip == 0){
    cprintf("swapin: pid %d has no swap file\n", pid);
    return -1;
  }

  // find slot for this va
  int slot = -1;
  for(int i = 0; i < MAX_SWAP_SLOTS; i++){
    if(p->swaptable[i].used && p->swaptable[i].va == va){
      slot = i;
      break;
    }
  }
  if(slot < 0){
    cprintf("swapin: no slot for pid %d va 0x%lx\n", pid, va);
    return -1;
  }

  uint off = (uint)slot * PGSIZE;
  begin_op();
  int r = readi(p->swapip, page, off, PGSIZE);
  end_op();
  if(r != PGSIZE){
    cprintf("swapin: readi failed pid=%d slot=%d read=%d\n", pid, slot, r);
    return -1;
  }

  // free that slot entry
  p->swaptable[slot].used = 0;
  p->swaptable[slot].va = 0;
  p->swaptable[slot].slotno = -1;
  if(p->num_swapped > 0) p->num_swapped--;

  cprintf("swapin: pid %d va 0x%lx <- slot %d\n", pid, va, slot);
  return 0;
}

// Remove (delete) swap file and free metadata for a process
void
swapfree(int pid)
{
  struct proc *p = findproc(pid);
  if(p == 0) return;

  // clear swaptable
  int freed = 0;
  for(int i = 0; i < MAX_SWAP_SLOTS; i++){
    if(p->swaptable[i].used){
      p->swaptable[i].used = 0;
      p->swaptable[i].va = 0;
      p->swaptable[i].slotno = -1;
      freed++;
    }
  }
  p->num_swapped = 0;
  p->next_swap_slot = 0;

  if(p->swapip){
    // build filename
    char fname[32];
    swap_fname_for_pid(pid, fname, sizeof(fname));

    // close inode pointer if necessary
    // In vanilla xv6 we can just iput() it
    ilock(p->swapip);
    iput(p->swapip);  // iput will drop lock if held
    p->swapip = 0;

    begin_op();
    unlink(fname);
    end_op();

    if(freed > 0)
      cprintf("swapfree_proc: freed %d swap slots for pid %d and deleted %s\n", freed, pid, fname);
  }
}
