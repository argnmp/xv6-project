#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "procext.h"

extern struct pt ptable;

struct mlfq mlfq;

int
push_mlfq(struct proc* p, int target, int priority){
  struct proc** cur;
  struct proc** cur_limit;

  //cprintf("level %d, priority %d, cursor: %x ->", target, priority, get_mlfq_cur(target, priority));
  for(cur = get_mlfq_cur(target, priority), cur_limit = get_mlfq_cur_limit(target, priority); ; cur = get_mlfq_cur_next(cur, target)){
    if(*cur == 0){
      /** cprintf("push cur: %x", cur); */
      goto found;
    } 
    if(cur == cur_limit) break;
  }
  return -1;
  
found:
  //cprintf("%x, total: %d\n", cur, mlfq.l0_count);
  switch(target){
    case L0:
      mlfq.l0_count += 1;
      p->ticks = L0_tq;
      p->level = L0;
      break;
    case L1:
      mlfq.l1_count += 1;
      p->ticks = L1_tq;
      p->level = L1;
      break;
    case L2:
      mlfq.l2_count += 1;
      p->ticks = L2_tq;
      p->level = L2;
      break;
  }
  *cur = p;
  p->mlfq_pos = cur;
  
  return 0;
}

int
remove_mlfq(struct proc* p){
  switch(p->level){
    case L0:
      mlfq.l0_count -= 1;
    case L1:
      mlfq.l1_count -= 1;
    case L2:
      mlfq.l2_count -= 1;
  }
  *(p->mlfq_pos) = 0;
  return 0;
}

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

int update_mlfq(struct proc* p, int target, int priority){
  cprintf("pid: %d | (%d, %d) -> ",p->pid, p->level, p->priority );
  switch(target){
    case L0:
      p->level = L0;
      p->ticks = L0_tq;
      p->priority = priority;
      remove_mlfq(p);
      push_mlfq(p, L0, priority);
      break;
    case L1:
      p->level = L1;
      p->ticks = L1_tq;
      p->priority = priority;
      remove_mlfq(p);
      push_mlfq(p, L1, priority);
      break;
    case L2:
      p->level = L2;
      p->ticks = L2_tq;
      p->priority = priority;
      remove_mlfq(p);
      push_mlfq(p, L2, priority);
      break;
  }
  cprintf("(%d, %d)\n", p->level, p->priority);
  return 0; 
}
int reschedule_mlfq(struct proc* p){
  if(p->level == L0){
    update_mlfq(p, L1, p->priority);
  }
  else if(p->level == L1){
    update_mlfq(p, L2, p->priority);
  }
  else if(p->level == L2){
    if(p->priority > 0){
      update_mlfq(p, L2, p->priority - 1);
    }
    else if(p->priority == 0){
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

int boost_mlfq(){
  cprintf("boost start!\n");
  struct proc** cur;
  struct proc** cur_limit;

  for(cur = get_mlfq_cur(L1, PNONE), cur_limit = get_mlfq_cur_limit(L1, PNONE); ; cur = get_mlfq_cur_next(cur, L1)){
    if((*cur)!=0){
      update_mlfq(*cur, L0, 3);     
      (*cur) = 0;
    }
    if(cur == cur_limit) {
      break;
    }
  }

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
  cprintf("boost end!\n");
  return 0;
}


/*
 * system call
 */

int getLevel(void){
  struct proc *curproc = myproc();
  return curproc->level;
}
int setPriority(int pid, int priority){
  return proc_setPriority(pid, priority);     
}

