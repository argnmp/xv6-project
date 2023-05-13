#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

// int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg){
//   return 0; 
// }
// void thread_exit(void *retval){
//
// }
// int thread_join(thread_t thread, void **retval){
//   return 0;
// }

int sys_thread_create(void){
  thread_t* thread;
  void* (*start_routine)(void*);
  void* arg;
  if(argptr(0, (char**)&thread, sizeof(thread_t)) < 0 || argptr(1, (char**)&start_routine, sizeof(void*)) < 0 || argptr(2, (char**)&arg, sizeof(void*))){
    return -1;
  }
  // cprintf("%d, %d, %d \n", *thread, start_routine, *(int*)arg);  
  // cprintf("%d", (int)start_routine(arg));
  //start_routine(arg);
  int res = thread_create(thread, start_routine, arg);
  //procdump();
  return res;
}
void sys_thread_exit(void){
  void* retval;
  if(argptr(0, (char**)&retval, sizeof(void*)) < 0){
    return;
  }
  
}
int sys_thread_join(void){
  thread_t thread;
  void* retval_p;
  if(argptr(0, (char**)&thread, sizeof(thread_t)) < 0 || argptr(1, (char**)&retval_p, sizeof(void*)) < 0){
    return -1;
  }
  return 0;
}
