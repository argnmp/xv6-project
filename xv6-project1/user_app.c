#include "types.h"
#include "stat.h"
#include "user.h"
int main(int argc, char *argv[]) {
  //__asm__("int $128");
  for(int i = 0; i<3; i++){
    int pid = fork();
    if(pid == 0){
      setPriority(getpid(), (getpid()+3)%4);
      //int level = getLevel();
      for(;;){
        //int cur_level = getLevel();   
        /** if(level != cur_level) */
          /** printf(0, "changed! pid: %d | (%d, %d) -> (%d, %d)\n",pid, level/10, level%10, cur_level/10, cur_level%10); */
        //level = cur_level;
        yield();
        schedulerLock(2019097210);
        schedulerUnlock(2019097210);
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


