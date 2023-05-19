#include "types.h"
#include "stat.h"
#include "user.h"
#define dbg(fmt, args...) printf(1, "[%d: %s] pid %d | " fmt "\n",__LINE__, __FUNCTION__, getpid(), ##args)
#define WORKER1 5
#define WORKER2 5
int cal2[WORKER2] = {0};
void* temp2(void* args){
  int* ret = 0;
  for(int i = 0; i<10; i++){
    cal2[(int)args] += (int)args;
  }
  thread_exit(&ret);
  return 0;
}
void* temp1(void* args){
  //printf(1, "thread: arg: %d", (int)args);
  int* ret;
  int pid = fork();
  if(pid==0){
    int retval = 0;
    thread_t tids[WORKER2] = {0,};
    for(int i = 0; i<WORKER2; i++){
      int res = thread_create(&tids[i], temp2, (int*)i);
      if(res < 0)
        dbg("thc failed");
    }
    int v = 0; 
    for(int i = 0; i<WORKER2; i++){
      thread_join(tids[i], (void*)&retval);
      v += cal2[i];  
    }
    dbg("result: %d", v);
    exit();
  }
  else if(pid < 0){
      dbg("fork failed");
  }
  else {
    wait();
  }
  thread_exit(&ret); 
  return 0;
}
int main(int argc, char * argv[]){
  int retval = 0;
  thread_t tids[WORKER1] = {0,};
  for(int i = 0; i<WORKER1; i++){
    int res = thread_create(&tids[i], temp1, (int*)i);
    if(res < 0)
      dbg("thc failed");
  }
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
  }
  exit();
}
