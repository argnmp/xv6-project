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
  exit();
}
