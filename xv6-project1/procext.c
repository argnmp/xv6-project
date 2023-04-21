#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "procext.h"

// trap.c
extern int schedticks;
extern struct spinlock schedtickslock;

/*
 * mlfq and lock for mlfq
 * all scheduling actions are occured in here
 */
struct mlfq mlfq;
struct spinlock mlfq_lock;


/*
 * push to target queue in mlfq
 */
int
push_mlfq(struct proc* p, int target, int priority){
  struct proc** cur;
  struct proc** cur_limit;

  // find empty space and push the address of process
  for(cur = get_mlfq_cur(target, priority), cur_limit = get_mlfq_cur_limit(target, priority); ; cur = get_mlfq_cur_next(cur, target)){
    if(*cur == 0){
      goto found;
    } 
    if(cur == cur_limit) break;
  }
  return -1;
  
found:
  *cur = p;
  p->mlfq_pos = cur;
  
  return 0;
}

/*
 * push to the front of the target queue
 * rearrange the processes in queue
 */
int push_mlfq_front(struct proc* p, int target, int priority){
  if(target == L2) return -1;
  struct proc** cur;
  struct proc** cur_limit;

  int cursor = 0;
  struct proc* buffer[NPROC];
  buffer[cursor] = p;
  cursor += 1;

  // save all the processes temprorarily
  //-cprintf("reserve pid: %d\n", buffer[0]->pid);
  for(cur = get_mlfq_cur(target, priority), cur_limit = get_mlfq_cur_limit(target, priority); ; cur = get_mlfq_cur_next(cur, target)){
    if(*cur != 0){
      //-cprintf("reserve pid: %d\n", (*cur)->pid);
      buffer[cursor] = *cur;
      *cur = 0;
      cursor += 1;
    }
    if(cur == cur_limit) break;
  }
  /* 
   * If priority boosting occurs while holding lock, it boosts all the processes to L0, and set the process that has been holding lock to be served first in L0 queue.
   * For MLFQ_SCH_SCHEME 0, when this happens, always select the new process.
   * So the queue cursor which means the next process to be served is set to 0.
   * For MLFQ_SCH_SCHEME 1, when this happens, it does not yield to scheduler. It just returns and go back to the process that has been executed(which time quantum was reset).
   * So, the L0 queue cursor which means the next process to be served is set to 1.(because the cpu is not yielded to scheduler from process)
   */
  switch(target){
    case L0:
      if(MLFQ_SCH_SCHEME==0){
        mlfq.l0_cur = 0;
      } 
      else if(MLFQ_SCH_SCHEME==1){
        mlfq.l0_cur = 1;
      }
      else {
        panic("MLFQ_SCH_SCHEME parameter is invalid");
      }
      // move the processes
      for(int i = 0; i<cursor; i++){
        mlfq.l0[i] = buffer[i]; 
        buffer[i]->mlfq_pos = &mlfq.l0[i];
      }
      p->mlfq_pos = mlfq.l0;
      break;
    case L1:
      if(MLFQ_SCH_SCHEME==0){
        mlfq.l1_cur = 0;
      } 
      else if(MLFQ_SCH_SCHEME==1){
        mlfq.l1_cur = 1;
      }
      else {
        panic("MLFQ_SCH_SCHEME parameter is invalid");
      }
      // move the processes
      for(int i = 0; i<cursor; i++){
        mlfq.l1[i] = buffer[i]; 
        buffer[i]->mlfq_pos = &mlfq.l1[i];
      }
      p->mlfq_pos = mlfq.l1;
      break;
    case L2:
      return -1;
      break;
  }
  
  return 0;
}

/*
 * remove process from mlfq queue
 */
int
remove_mlfq(struct proc* p){
  *(p->mlfq_pos) = 0;
  return 0;
}

/*
 * get the current address of target mlfq queue that the cursor indicates
 */
struct proc**
get_mlfq_cur(int target, int priority){
  struct proc** ret;
  if(target == L0){
    ret = mlfq.l0 + mlfq.l0_cur;
  }
  else if(target == L1){
    ret = mlfq.l1 + mlfq.l1_cur;
  }
  else {
    ret = mlfq.l2 + mlfq.l2_cur[priority];
  }
  return ret;
}

/*
 * get the address of the last index in target queue that the cursor indicates
 * address of index i-1 if the cursor points to index i in circular queue
 */
struct proc**
get_mlfq_cur_limit(int target, int priority){
  if(target == 0){
    struct proc** ret = (mlfq.l0_cur==0) ? mlfq.l0 + (L0_NPROC-1) : mlfq.l0 + mlfq.l0_cur - 1;
    return ret;
  }
  else if(target == 1){
    struct proc** ret = (mlfq.l1_cur==0) ? mlfq.l1 + (L1_NPROC-1) : mlfq.l1 + mlfq.l1_cur - 1;
    return ret;
  }
  else {
    struct proc** ret = (mlfq.l2_cur[priority]==0) ? mlfq.l2 + (L2_NPROC-1) : mlfq.l2 + mlfq.l2_cur[priority] - 1;
    return ret;
  }
}

/*
 * get the next address of cursor(by address)
 * if the given address is the last index of queue, it returns the address of first index because the queue is circular
 */
struct proc**
get_mlfq_cur_next(struct proc** cur, int target){
  if(target == 0){
    struct proc** ret = (&mlfq.l0[L0_NPROC-1]==cur) ? &mlfq.l0[0] : cur + 1;
    return ret;
  }
  else if(target == 1){
    struct proc** ret = (&mlfq.l1[L1_NPROC-1]==cur) ? &mlfq.l1[0] : cur + 1;
    return ret;
  }
  else {
    struct proc** ret = (&mlfq.l2[L2_NPROC-1]==cur) ? &mlfq.l2[0] : cur + 1;
    return ret;
  }
}

/*
 * move the cursor of target queue to next in circular queue manner
 */ 
struct proc**
move_mlfq_cur(int target, int priority){
  struct proc** ret;
  if(target == 0){
    mlfq.l0_cur += 1;
    mlfq.l0_cur %= L0_NPROC;     
    ret = mlfq.l0 + mlfq.l0_cur;
  }
  else if(target == 1){
    mlfq.l1_cur += 1;
    mlfq.l1_cur %= L1_NPROC;     
    ret = mlfq.l1 + mlfq.l1_cur;
  }
  else {
    mlfq.l2_cur[priority] += 1;
    mlfq.l2_cur[priority] %= L2_NPROC;     
    ret = mlfq.l2 + mlfq.l2_cur[priority];
  }
  return ret;
}

// reset the time quantum of the process which currently belonging to
int reset_mlfq_tq(struct proc* p){
  switch(p->level){
    case L0:
      p->ticks = L0_tq;
      break;
    case L1:
      p->ticks = L1_tq;
      break;
    case L2:
      p->ticks = L2_tq;
      break;
  }
  return 0;
}

// move the process to the target queue and priority
int update_mlfq(struct proc* p, int target, int priority){
  //-cprintf("pid: %d | (%d, %d) -> ",p->pid, p->level, p->priority );
  switch(target){
    case L0:
      p->level = L0;
      p->ticks = L0_tq;
      p->priority = priority;
      /*
       * remove from the current position and push to the target queue
       */
      remove_mlfq(p);
      push_mlfq(p, L0, priority);
      break;
    case L1:
      p->level = L1;
      p->ticks = L1_tq;
      p->priority = priority;
      /*
       * remove from the current position and push to the target queue
       */
      remove_mlfq(p);
      push_mlfq(p, L1, priority);
      break;
    case L2:
      p->level = L2;
      p->ticks = L2_tq;
      p->priority = priority;
      /*
       * remove from the current position and push to the target queue
       */
      remove_mlfq(p);
      push_mlfq(p, L2, priority);
      break;
  }
  //-cprintf("(%d, %d)\n", p->level, p->priority);
  return 0; 
}

/*
 * move the process to another queue when time quantum is all consumed
 */
int reschedule_mlfq(struct proc* p){
  if(p->level == L0){
    // if current level is L0, move to L1
    update_mlfq(p, L1, p->priority);
  }
  else if(p->level == L1){
    // if current level is L1, move to L2
    update_mlfq(p, L2, p->priority);
  }
  else if(p->level == L2){
    // if current level is L2, push again to the L2 with decreased priority
    if(p->priority > 0){
      update_mlfq(p, L2, p->priority - 1);
    }
    else if(p->priority == 0){
      // if current priority is 0, push again to the L2 with priority 0
      update_mlfq(p, L2, p->priority); 
    }
    else {
      return -1;
    }
  }
  else {
    return -1;
  }
  return 0;
}
/*
 * push the process again to the current queue
 */
int reschedule_mlfq_to_last(struct proc* p){
  //-cprintf("pid: %d | (%d, %d) -> ",p->pid, p->level, p->priority );
  if(p->level == L0){
      remove_mlfq(p);
      push_mlfq(p, L0, p->priority);
  }
  else if(p->level == L1){
      remove_mlfq(p);
      push_mlfq(p, L1, p->priority);
  }
  else if(p->level == L2){
      remove_mlfq(p);
      push_mlfq(p, L2, p->priority);
  }
  else {
    return -1;
  }
  //-cprintf("(%d, %d)\n", p->level, p->priority);
  return 0;
}

/*
 * priority boost
 * this function is called by trap() when the schedticks reaches 100
 */
int boost_mlfq(){
  //-cprintf("boost start!\n");
  struct proc** cur;
  struct proc** cur_limit;

  // reset the time quantum and priority
  for(cur = get_mlfq_cur(L0, PNONE), cur_limit = get_mlfq_cur_limit(L0, PNONE); ; cur = get_mlfq_cur_next(cur, L0)){
    if((*cur)!=0){
      (*cur)->ticks = L0_tq;
      (*cur)->priority = 3;
    }
    if(cur == cur_limit) {
      break;
    }
  }

  // move all processes in L1 to L0
  for(cur = get_mlfq_cur(L1, PNONE), cur_limit = get_mlfq_cur_limit(L1, PNONE); ; cur = get_mlfq_cur_next(cur, L1)){
    if((*cur)!=0){
      update_mlfq(*cur, L0, 3);     
      (*cur) = 0;
    }
    if(cur == cur_limit) {
      break;
    }
  }

  /*
   * move all process in L2 to L0 by the order of the priority
   */
  for(cur = get_mlfq_cur(L2, P0), cur_limit = get_mlfq_cur_limit(L2, P0); ; cur = get_mlfq_cur_next(cur, L2)){
    if((*cur)!=0 && (*cur)->priority == P0){
      update_mlfq(*cur, L0, 3);     
      (*cur) = 0;
    }
    if(cur == cur_limit) break;
  }
  for(cur = get_mlfq_cur(L2, P1), cur_limit = get_mlfq_cur_limit(L2, P1); ; cur = get_mlfq_cur_next(cur, L2)){
    if((*cur)!=0 && (*cur)->priority == P1){
      update_mlfq(*cur, L0, 3);     
      (*cur) = 0;
    }
    if(cur == cur_limit) break;
  }
  for(cur = get_mlfq_cur(L2, P2), cur_limit = get_mlfq_cur_limit(L2, P2); ; cur = get_mlfq_cur_next(cur, L2)){
    if((*cur)!=0 && (*cur)->priority == P2){
      update_mlfq(*cur, L0, 3);     
      (*cur) = 0;
    }
    if(cur == cur_limit) break;
  }
  for(cur = get_mlfq_cur(L2, P3), cur_limit = get_mlfq_cur_limit(L2, P3); ; cur = get_mlfq_cur_next(cur, L2)){
    if((*cur)!=0 && (*cur)->priority == P3){
      update_mlfq(*cur, L0, 3);     
      (*cur) = 0;
    }
    if(cur == cur_limit) break;
  }
  //-cprintf("boost end!\n");
  return 0;
}


/*
 * system call
 */

// return the current level of the process
int getLevel(void){
  struct proc *curproc = myproc();
  return curproc->level;
}

/*
 * set the priority of the process
 * this simply calls proc_setPriority in proc.c
 */
int setPriority(int pid, int priority){
  return proc_setPriority(pid, priority);     
}

/*
 * acquire scheduler lock
 */
int schedulerLock(int password){
  struct proc *curproc = myproc();

  // if password is invalid return 1
  if(password != 2019097210){
    acquire(&schedtickslock);
    int used_ticks;
    if(curproc->level == L0){
      used_ticks = 4 - curproc->ticks + 1;
    }
    else if(curproc->level == L1){
      used_ticks = 6 - curproc->ticks + 1;
    }
    else {
      used_ticks = 8 - curproc->ticks + 1;
    }
    // print the pid, time quantum, level
    cprintf("schedulerLock invalid password | pid: %d, time_quantum: %d, level: %d\n", curproc->pid, used_ticks, curproc->level);
    release(&schedtickslock);
    return 1;
  }

  // acquire mlfq_lock because this is not protected by ptable.lock
  acquire(&mlfq_lock);
  if(mlfq.locking_pid == 0){
    // no process is holding scheduler lock

    // set the locking_pid to the curent process
    mlfq.locking_pid = curproc->pid;
    release(&mlfq_lock);

    // reset the globaltick to 0
    acquire(&schedtickslock);
    schedticks = 0;
    release(&schedtickslock);

    return 0;
  }
  else if(mlfq.locking_pid == curproc->pid){
    // current process is holding scheduler lock
    release(&mlfq_lock);
    return 0;
  }
  else{
    // other process is holding scheduler lock. It's impossible to acquire
    release(&mlfq_lock);
    return -1;
  }
}
/*
 * acquire scheduler unlock
 */
int schedulerUnlock(int password){
  struct proc *curproc = myproc();
  // if password is invalid return 1
  if(password != 2019097210){
    acquire(&schedtickslock);
    int used_ticks;
    if(curproc->level == L0){
      used_ticks = 4 - curproc->ticks + 1;
    }
    else if(curproc->level == L1){
      used_ticks = 6 - curproc->ticks + 1;
    }
    else {
      used_ticks = 8 - curproc->ticks + 1;
    }
    // print the pid, time quantum, level
    cprintf("schedulerUnlock invalid password | pid: %d, time_quantum: %d, level: %d\n", curproc->pid, used_ticks, curproc->level);
    release(&schedtickslock);
    return 1;
  }

  // acquire mlfq_lock because this is not protected by ptable.lock
  acquire(&mlfq_lock);
  if(mlfq.locking_pid != 0 && mlfq.locking_pid == curproc->pid){
    // current process is holding lock.
    // it's okay to unlock the scheduler lock
    
    // reset the locking_pid
    mlfq.locking_pid = 0;

    // move current running process to L0
    curproc->level = L0;
    curproc->ticks = L0_tq;
    curproc->priority = 3;

    remove_mlfq(curproc);
    push_mlfq_front(curproc, L0, 3);

    /*
     * For syscall, there is no yield occurs after handling syscall request but it just resumes the process that was running before syscall
     * So, move mlfq L0 cursor by 1, so that the next process could be executed when timer interrupt occurs.
     */
    mlfq.l0_cur += 1;

    release(&mlfq_lock);
    return 0;
  }
  else {
    // scheduler lock is not set or other process is holding scheduler lock
    // cannot unlock the scheduler lock
    release(&mlfq_lock);
    return -1;
  }
}

/*
 * debugging function
 * show the current state of the mlfq
 * '*' indicates the each space in queue
 */
void view_mlfq_status(){
  cprintf("l0_cur: %d, l1_cur: %d, l2_p0_cur: %d, l2_p1_cur: %d, l2_p2_cur: %d, l2_p3_cur: %d\n", mlfq.l0_cur, mlfq.l1_cur, mlfq.l2_cur[0], mlfq.l2_cur[1], mlfq.l2_cur[2], mlfq.l2_cur[3]);
  for(int i = 0; i<L0_NPROC; i++){
    if(mlfq.l0[i]==0) cprintf("* ");
    else cprintf("%d ",mlfq.l0[i]->pid);
  }
  cprintf("\n");
  for(int i = 0; i<L1_NPROC; i++){
    if(mlfq.l1[i]==0) cprintf("* ");
    else cprintf("%d ",mlfq.l1[i]->pid);
  }
  cprintf("\n");
  for(int i = 0; i<L2_NPROC; i++){
    if(mlfq.l2[i]==0) cprintf("* ");
    else cprintf("%d ",mlfq.l2[i]->pid);
  }
  cprintf("\n");
}

