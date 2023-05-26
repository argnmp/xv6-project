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

int growthread(struct proc* target, uint sz_org, int n, uint* sz_ptr)
{
  /*
   * just allocate pages to destination
   * does not change thread values
   */
  uint sz = sz_org;
  if(target->sz_limit!=0 && sz+n > target->sz_limit){
    return -1;
  }
  if(n > 0){
    if((sz = allocuvm(target->pgdir, sz, sz + n)) == 0){
      return -1;
    }
  } else if(n < 0){
    if((sz = deallocuvm(target->pgdir, sz, sz + n)) == 0){
      return -1;
    }
  }
  *sz_ptr = sz;
  return 0;
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
uint load_thmem(struct proc* np){
  if(np->th.main == np) panic("load thmem");

  uint sz = PGROUNDUP(np->th.main->sz); 

  if(np->th.main->thstack_sp == np->th.main->thstack_fp){
    if(growthread(np->th.main, sz, 2*PGSIZE, &sz) == -1){
      return 0;
    }
    clearpteu(np->pgdir, (char*)(sz - 2*PGSIZE));
    np->th.main->sz = sz;
    np->th.main->ssz += 2*PGSIZE;
  }
  else {
    sz = ((uint*)np->th.main->thstack_sp)[0];
    np->th.main->thstack_sp += 4; 
  }
  return sz;
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
void delayed_exit(struct proc* curproc, struct proc* target){
  target->delayed_exit = 1;
  target->delayed_exit_addr = curproc;
  target->killed = 1;
  wakeup1_wrapper(target);
  sleep_wrapper(curproc);   
}


