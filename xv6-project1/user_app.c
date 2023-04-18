/*
#include "types.h"
#include "stat.h"
#include "user.h"
void create_worker(int n){
  for(int i = 0; i<n; i++){
    int pid = fork();
    setPriority(getpid(), 0);
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
  create_worker(4);

  schedulerLock(2019097210);
  for(int i = 0;;i++){
    if(i%64==0){
      //printf(0,".\n");
    }
  }
  schedulerUnlock(2019097210);
  for(int i = 0; i<200; i++){
    wait();
  }
  exit();
}
*/
#include "types.h"
#include "stat.h"
#include "user.h"
void create_worker(int n){
  for(int i = 0; i<n; i++){
    int pid = fork();
    if(pid==0){
       
      //printf(0, "pid: %d\n", getpid());
      for(int k = 0; k<1000000; k++){
        printf(0,"");
      }

      schedulerLock(2019097210);
      printf(0, "finish process sequence i: %d\n", i);
      schedulerUnlock(2019097210);
      exit();
    }
    else if(pid>0){
      //setPriority(pid, 1);
    }
    else{
      exit();
    }
  }
}
int main(int argc, char * argv[]){
  printf(0,"start function\n");
  schedulerLock(209097210);
  printf(0, "after schedulerLock\n");
  create_worker(3);
  printf(0, "before schedulerUnlock\n");
  schedulerUnlock(2019097210);
  for(int i = 0; i<62; i++){
    wait();
  }
  exit();
}
