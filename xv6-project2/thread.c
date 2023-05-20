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
struct spinlock thstacklk;
void init_thstacklk(){
  initlock(&thstacklk, "thstacklk");
}

int save_thmem(struct proc* curthread, struct proc* target){
  //acquire(&thstacklk);
  if(target->thstack_sp == target->thstack) {
    //release(&thstacklk);
    return -1;
  }
  target->thstack_sp -= 4; 
  ((uint*)target->thstack_sp)[0] = curthread->sz;
  //release(&thstacklk);
  return 0;
}
uint load_thmem(struct proc* curthread){
  //acquire(&thstacklk);
  uint p;
  if(curthread->th.main->thstack_sp == curthread->th.main->thstack_fp){
    // cprintf("return 0\n");
    //release(&thstacklk);
    return 0; 
  }
  else {
    p = ((uint*)curthread->th.main->thstack_sp)[0];
    curthread->th.main->thstack_sp += 4; 
  }
  //release(&thstacklk);
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
  p->delayed_exit = 0;
  // add thread's address space to process's thstack space for later use
  save_thmem(p, p->th.main);
}
