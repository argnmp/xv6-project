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
  return (void*)(10*(*(int*)args));
}
int main(int argc, char * argv[]){
  thread_t tid = 777;
  int arg = 123;
  thread_create(&tid, print_hello, &arg);
  exit();
}
