#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "thread.h"
#include "proc.h"
#include "spinlock.h"

int next_tid = 1;

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg){
    
  return 0; 
}
void thread_exit(void *retval){

}
int thread_join(thread_t thread, void **retval){
  return 0;
}

