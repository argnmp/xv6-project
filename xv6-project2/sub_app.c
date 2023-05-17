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
int calc[WORKER] = {0,};
int is_done = 0;
void* calculate(void* args){
  //printf(1, "working on thread: %d\n", *(int*)args);
  for(int i = 1; i<=500000; i++){
    *(int*)args += 1;
  }
  int* ret = (int*)40;
  //*ret = 1293123;
  for(;;);
  thread_exit((void*)ret);
  return 0;
}
void* temp(void* args){
  //printf(1, "thread: arg: %d", (int)args);
  int* ret = (int*)40;
  if((int)args % 2 == 0){
    thread_exit(&ret); 
    for(;;);
  } 
  else {
    for(;;);
  }
  return 0;
}
int main(int argc, char * argv[]){
  thread_t tid;
  thread_t tids[WORKER] = {0,};
  int* retval;
  for(int i = 0; i<WORKER; i++){
    int res = thread_create(&tid, temp, (int*)i);
    tids[i] = tid;
    printf(1, "create thread %d | is succeeded? %d\n",tids[i], res);

    if(i%2==0){
      res = thread_join(tids[i], (void*)&retval);
      printf(1, "join thread %d | is succeeded? %d | retval: %d \n",tids[i], res, *(int*)retval);
    }
  }
  for(;;);
  exit();
  // for(int i = 0; i<WORKER; i++){
  //   int res = thread_join(tids[i], (void*)&retval);
  //   printf(1, "join thread %d | is succeeded? %d | retval: %d \n",i, res, *(int*)retval);
  // }
  // for(int i = 0; i<WORKER; i++){
  //   printf(1, "worker %d, value: %d\n", i, calc[i]);
  // }
  //for(;;);
}
