#include "types.h"
#include "stat.h"
#include "user.h"
/*
int main(int argc, char *argv[]) {
  //__asm__("int $128");
  for(int i = 0; i<3; i++){
    int pid = fork();
    if(pid == 0){
      setPriority(getpid(), (getpid()+3)%4);
      int level = getLevel();
      schedulerLock(2019097210);
      for(;;){
        int cur_level = getLevel();
        if(level != cur_level)
          printf(0, "changed! pid: %d | (%d, %d) -> (%d, %d)\n",getpid(), level/10, level%10, cur_level/10, cur_level%10);
        level = cur_level;
        //yield();
        printf(0, "pid: %d\n", getpid());
        
        //printf(0, "hello there?");
      }
      exit();
    }
    else if(pid > 0){
    }
    else {
      printf(0, "forkfailed\n");
    }
  }
  for(int i = 0; i<2; i++){
    wait();
  }
  printf(0, "waitcomplete\n");
  exit();
}
*/
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
int main(int argc, char * argv[]){
  int pid;
  pid = fork();
  if(pid==0){
    schedulerLock(2019097210);
    //printf(0, "pid: %d\n", getpid());
    
    for(;;){
      //printf(0, "still, pid: %d\n", getpid());
    }
    exit();
  }
  else if(pid>0){
    printf(0, "main\n");
    create_worker(50);
    for(;;);
  }
  else{
    exit();
  }
  /** pid = fork(); */
  /** if(pid == 0){ */
    /** for(int i = 0;; i++){ */
      /** printf(0,"pid: %d, %d\n", getpid(), i);  */
    /** } */
  /** } */

  for(int i = 0; i<200; i++){
    wait();
  }
  exit();
}

