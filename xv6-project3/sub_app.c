#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

/* int main(int argc, char * argv[]){
  int fd = open("tf", O_RDWR);
  if(fd<0){
    udbg("file does not opened");
    exit();
  }
  char buf[100] = {0,};
  if(read(fd, buf, 10)!= 10){
    udbg("write file failed"); 
    close(fd);
    exit();
  }  
  udbg("read result: %s", buf);
  close(fd);
  exit();
} */
int main(int argc, char* argv[]){
  char* str = "worldhello";
  int fd = open("test_file",O_RDWR);
  if(fd<0){
    udbg("create file failed");
    exit();
  }
  if(write(fd, str, strlen(str))!=strlen(str)){
      udbg("write file failed"); 
      close(fd);
      exit();
  }
  int res = sync();
  udbg("flush count: %d", res);
  close(fd);
  exit();
}
