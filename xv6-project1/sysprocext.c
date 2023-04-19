#include "types.h"
#include "defs.h"
#include "procext.h"
/*
 * wrapper functions for the system call
 */

void sys_yield(void){
  // simply calls yield() in proc.c
  yield();
}
int sys_getLevel(void){
  // simply calls getLevel() in procext.c
  return getLevel(); 
}
void sys_setPriority(void){
  int pid;
  int priority;
  /*
   * check if the argument is given
   */
  if(argint(0, &pid) < 0)
    exit();
  if(argint(1, &priority) < 0)
    exit();
  if(priority < P0 || priority > P3)
    exit();

  // update the priority of process
  int res = setPriority(pid, priority);  
  // if it is invalid access to other process, exit the process
  if(res == -1) exit();
  
}
void sys_schedulerLock(void){
  int password; 
  /*
   * check if the argument is given
   */
  if(argint(0, &password) < 0)
    exit();
  int res = schedulerLock(password);
  if(res == -1){
    // other process is holding scheduler lock
    cprintf("other process is holding scheduler lock");
  }
  else if(res == 1){
    // password is incorrect
    exit();
  }
}
void sys_schedulerUnlock(void){
  int password;
  /*
   * check if the argument is given
   */
  if(argint(0, &password) < 0)
    exit();
  int res = schedulerUnlock(password);

  // password is incorrect
  if(res == 1) exit();
}
