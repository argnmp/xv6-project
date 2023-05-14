#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "thread.h"
#include "proc.h"
#include "spinlock.h"

/* should acquire ptable.lock */

int save_thmem(struct proc* curthread){
  if(curthread->th.main->thstack_sp == curthread->th.main->thstack) return -1;
  curthread->th.main->thstack_sp -= 4; 
  ((uint*)curthread->th.main->thstack_sp)[0] = curthread->sz;
  // cprintf("sz: %d, s: %d\n",curthread->sz, *((uint*)curthread->th.main->thstack_sp));
  // cprintf("save_thmem: thstack bottom: %d, thstack_sp: %d, thstack_fp: %d\n", curthread->th.main->thstack, curthread->th.main->thstack_sp, curthread->th.main->thstack_fp);   
  return 0;
}
uint load_thmem(struct proc* curthread){
  uint p;
  if(curthread->th.main->thstack_sp == curthread->th.main->thstack_fp){
    // cprintf("return 0\n");
    return 0; 
  }
  else {
    p = ((uint*)curthread->th.main->thstack_sp)[0];
    curthread->th.main->thstack_sp += 4; 
  }
  // cprintf("load_thmem: thstack bottom: %d, thstack_sp: %d, thstack_fp: %d\n", curthread->th.main->thstack, curthread->th.main->thstack_sp, curthread->th.main->thstack_fp);   
  // cprintf("p: %d\n", p);
  return p;
}
void remove_th(struct proc* p){
  // free all resources
  kfree(p->kstack);
  p->kstack = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->killed = 0;
  p->state = UNUSED;
  // add thread's address space to process's thstack space for later use
  save_thmem(p);
}
