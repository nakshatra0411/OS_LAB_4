#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "mru.h"

extern struct proc proc[NPROC];

uint64
sys_getpagestat(void)
{
  int pid;
  uint64 st_addr;
  
  argint(0, &pid);
  argaddr(1, &st_addr);
  
  struct proc *p = 0;
  
  // Find process
  for(struct proc *pp = proc; pp < &proc[NPROC]; pp++) {
    if(pp->pid == pid) {
      p = pp;
      break;
    }
  }
  
  if(p == 0)
    return -1;
  
  struct pagestat st;
  st.page_faults = p->page_faults;
  st.swap_ins = p->swap_ins;
  st.swap_outs = p->swap_outs;
  
  if(copyout(myproc()->pagetable, st_addr, (char*)&st, sizeof(st)) < 0)
    return -1;
  
  return 0;
}

uint64
sys_dumpmru(void)
{
  mrudump();
  return 0;
}


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
