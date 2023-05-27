#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

/*
 * these are wapper functions for thread_create, thread_join, thread_exit
 */

int sys_thread_create(void){
  thread_t* thread;
  void* (*start_routine)(void*);
  void* arg;
  if(argptr(0, (char**)&thread, sizeof(thread_t)) < 0 || argptr(1, (char**)&start_routine, sizeof(void*)) < 0 || argptr(2, (char**)&arg, sizeof(void*))){
    return -1;
  }
  int res = thread_create(thread, start_routine, arg);
  return res;
}
void sys_thread_exit(void){
  void* retval;
  if(argptr(0, (char**)&retval, sizeof(void*)) < 0){
    return;
  }
  thread_exit(retval);
  
}
int sys_thread_join(void){
  thread_t thread;
  void* retval_p;
  if(argint(0, &thread) < 0 || argptr(1, (char**)&retval_p, sizeof(void*)) < 0){
    return -1;
  }
  return thread_join(thread, retval_p);
}
