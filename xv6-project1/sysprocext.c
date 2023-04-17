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
void sys_setPriority(void){
  int pid;
  int priority;
  if(argint(0, &pid) < 0)
    exit();
  if(argint(1, &priority) < 0)
    exit();
  if(priority < P0 || priority > P3)
    exit();
  int res = setPriority(pid, priority);  
  if(res == -1) exit();
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
