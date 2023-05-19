#include "types.h"
#include "stat.h"
#include "user.h"
#define dbg(fmt, args...) printf(1, "[%d: %s] pid %d | " fmt "\n",__LINE__, __FUNCTION__, getpid(), ##args)
#define WORKER 20
#define WORKER2 1

void* temp2(void* args){
  int* ret = (int*)((int)200+(int)args);
  // dbg("temp2");
  thread_exit(ret);
  return 0;
}
void* temp1(void* args){
  int* ret = (int*)((int)200+(int)args);
  int pid = fork();
  // dbg("temp1");

  if(pid == 0){
    // dbg("in th fork");

    int /* res,  */retval = 0;
    thread_t tids[WORKER2] = {0,};
    for(int i = 0; i<WORKER2; i++){
      thread_create(&tids[i], temp2, (int*)i);
    }
    for(int i = 0; i<WORKER2; i++){
      /* res =  */thread_join(tids[i], (void*)&retval);
      // if(res==-1)
      //   dbg("th join -> tid: %d, res: %d, retval: %d", tids[i], res, retval);
    }
    exit();
  }
  // dbg("after fork wait");
  wait();
  thread_exit(ret);
  return 0;
}
int main(int argc, char * argv[]){
  int retval = 0;
  thread_t tids[WORKER] = {0,};
  for(int i = 0; i<WORKER; i++){
    /* int res =  */thread_create(&tids[i], temp1, (int*)i);
    /* int res =  */thread_join(tids[i], (void*)&retval);
    // dbg("th create, res: %d", res);
  }
  for(int i = 0; i<WORKER; i++){
    // dbg("th join, res: %d, retval: %d", res, retval);
  }
  //for(;;);
  exit();
}

