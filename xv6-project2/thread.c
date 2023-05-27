#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "thread.h"
#include "proc.h"
#include "spinlock.h"

int growthread(struct proc* target, uint sz_org, int n, uint* sz_ptr)
{
  /*
   * just allocate pages to destination
   * does not change thread values
   * code from growproc()
   */
  uint sz = sz_org;
  // if requested size exceeds memory limit, return -1
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
  
  if(target->thstack_sp == target->thstack) {
    // thmem space is full
    // return -1
    return -1;
  }
  // move thmem stack pointer
  target->thstack_sp -= 4; 
  // push address of thread stack space to thmem
  ((uint*)target->thstack_sp)[0] = curthread->sz;
  return 0;
}
uint load_thmem(struct proc* np){

  uint sz = PGROUNDUP(np->th.main->sz); 

  if(np->th.main->thstack_sp == np->th.main->thstack_fp){
    /*
     * no saved thread stack space exists.
     * allocate new thread stack space using the values of main thread(process itself)
     */
    if(growthread(np->th.main, sz, 2*PGSIZE, &sz) == -1){
      return 0;
    }
    clearpteu(np->pgdir, (char*)(sz - 2*PGSIZE));

    /*
     * update values of main thread(process)
     */
    np->th.main->sz = sz;
    np->th.main->ssz += 2*PGSIZE;
  }
  else {
    // saved thread stack space exists
    // pop address of saved thread stack space from thmem
    sz = ((uint*)np->th.main->thstack_sp)[0];
    // update thmem stack pointer
    np->th.main->thstack_sp += 4; 
  }
  return sz;
}
void remove_th(struct proc* p){
  // free all thread resources
  kfree(p->kstack);
  p->kstack = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->killed = 0;
  p->state = UNUSED;
  p->delayed_exit = 0;
  // save thread stack space of current thread in thmem space using save_thmem 
  save_thmem(p, p->th.main);
}
/*
 * delayed exit sequence
 */
void delayed_exit(struct proc* curproc, struct proc* target){
  /*
   * set delayed_exit and killed flag to 1
   * so that the target thread can be recognized in exit() syscall which is called by trap() because of killed flag
   * set delayed_exit_addr to current thread, so that the target thread can wakeup the requested thread(curproc)
   */
  target->delayed_exit = 1;
  target->delayed_exit_addr = curproc;
  target->killed = 1;
  // wakeup the target thread, so that it can be killed
  wakeup1_wrapper(target);
  // current thread sleeps, waiting for the target thread to be exited
  sleep_wrapper(curproc);   
}


