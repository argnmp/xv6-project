#include "types.h"
#include "stat.h"
#include "user.h"
/* void recall(int n){
  char a[1<<10];
  memset(a, -1, sizeof(a));
  printf(1, "%d / %d\n", a[0], n);
  recall(n+1);
} */
#define WORKER 10
void* temp(void* args){
  //printf(1, "thread: arg: %d", (int)args);
  int* ret = (int*)40;
  int pid = fork();
  if(pid==0){
    exit();
  }
  else if(pid < 0){
    printf(1, "fork failed\n");
  }
  else {
    wait();
  }
  thread_exit(&ret); 
  return 0;
}
int main(int argc, char * argv[]){
  int retval = 0;
  thread_t tid;
  thread_t tids[WORKER] = {0,};
  for(int i = 0; i<WORKER; i++){
    int res = thread_create(&tid, temp, (int*)i);
    if(res < 0)
      printf(1, "thread failed\n");
    tids[i] = tid;
  }
  for(int i = 0; i<WORKER; i++){
    thread_join(tids[i], (void*)&retval);
  }
  exit();
}
