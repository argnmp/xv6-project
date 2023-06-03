#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char * argv[]){
  int fd = open("tf", O_RDWR);
  if(fd<0){
    udbg("file does not opened");
    exit();
  }
  char* str = "worldhello";
  if(write(fd, str, strlen(str))!= strlen(str)){
    udbg("write file failed"); 
    close(fd);
    exit();
  }  
  close(fd);
  exit();
}
