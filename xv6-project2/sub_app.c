#include "types.h"
#include "stat.h"
#include "user.h"
/* void recall(int n){
  char a[1<<10];
  memset(a, -1, sizeof(a));
  printf(1, "%d / %d\n", a[0], n);
  recall(n+1);
} */

void* print_hello(void* args){
  printf(1, "working on thread: %d\n", *(int*)args);
  for(;;);
}
int main(int argc, char * argv[]){
  thread_t tid;
  printf(1, "function pointer addr: %d\n", print_hello);
  for(int i = 0; i<3; i++){
    int res = thread_create(&tid, print_hello, &i);
    printf(1, "create thread %d | is succeeded %d\n",tid, res);
  }
  for(;;);
  exit();
}
