#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define TARGET_FILE 16384
#define BSIZE 16384
#define LOOP TARGET_FILE/BSIZE
/* char buf[BSIZE] = {0,};
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
} */
int main(int argc, char* argv[]){
  char* str = "helloworld";
  int fd = open("test_file", O_CREATE | O_RDWR);
  if(fd<0){
    udbg("create file failed");
    exit();
  }
  if(write(fd, str, strlen(str))!=strlen(str)){
      udbg("write file failed"); 
      close(fd);
      exit();
  }
  close(fd);
  exit();
}
