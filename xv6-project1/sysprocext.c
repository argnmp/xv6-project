#include "types.h"
#include "defs.h"
#include "procext.h"
int sys_yield(void){
  yield();
  return 0;
}
int sys_getLevel(void){
  return getLevel(); 
}
int sys_setPriority(void){
  int pid;
  int priority;
  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &priority) < 0)
    return -1;
  if(priority < P0 || priority > P3)
    return -1;
  return setPriority(pid, priority);  
}
void sys_schedulerLock(void){
  int password; 
  if(argint(0, &password) < 0)
    exit();
  int res = schedulerLock(password);
  if(res == -1) exit();
}
void sys_schedulerUnlock(void){
  int password;
  if(argint(0, &password) < 0)
    exit();
  int res = schedulerUnlock(password);
  if(res == -1) exit();
}
