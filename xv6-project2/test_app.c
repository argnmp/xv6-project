#include "types.h"
#include "stat.h"
#include "user.h"
int main(int argc, char * argv[]){
  printf(1, "hello world\n");
  int i = fork();
  if(i==0){
    char *argv[1] = {"sub_app"};
    exec2(argv[0], argv, 10);
    printf(1, "exec failed\n");
  }
  wait();
  exit();
}

