#include "types.h"
#include "stat.h"
#include "user.h"
void create_worker(int n){
  for(int i = 0; i<n; i++){
    int pid = fork();
    if(pid==0){
      //schedulerUnlock(2019097210); 
      for(;;){
        //printf(0, "pid: %d\n", getpid());
      }
      exit();
    }
    else if(pid>0){

    }
    else{
      exit();
    }
  }
}
/*
int main(int argc, char *argv[]) {
  //__asm__("int $128");
  int worker_num = 4;
  for(int i = 0; i<worker_num; i++){
    int pid = fork();

    if(pid == 0){
      setPriority(getpid(), (getpid()+3)%4);
      //schedulerLock(2019097210);
      for(;;){
        
      }
      exit();
    }
    else if(pid > 0){
    }
    else {
      printf(0, "forkfailed\n");
    }
  }
  for(int i = 0; i<worker_num; i++){
    wait();
  }
  printf(0, "waitcomplete\n");
  exit();
}
*/
int main(int argc, char * argv[]){
  create_worker(3);
  for(int i = 0; i<200; i++){
    wait();
  }
  exit();
}

