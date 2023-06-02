#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#define dbg(fmt, args...) printf(1, "[%d: %s] pid %d | " fmt "\n",__LINE__, __FUNCTION__, getpid(), ##args)

#define TARGET_FILE 16777216
#define BSIZE 16384
#define LOOP TARGET_FILE/BSIZE
char buf[BSIZE] = {0,};
int main(int argc, char * argv[]){

  for(int i = 0; i<BSIZE; i++){
    buf[i] = 't';
  }
  int fd = open("test_file", O_CREATE | O_RDWR);
  if(fd<0){
    udbg("create file failed");
    exit();
  }
  for(int i = 0; i<LOOP; i++){
    if(write(fd, buf, sizeof(buf))!=sizeof(buf)){
      udbg("write file failed"); 
      close(fd);
      exit();
    }
  }
  
  close(fd);
  exit();
}
