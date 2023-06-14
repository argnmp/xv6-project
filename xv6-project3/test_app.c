#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char* argv[]){
  char* str = "helloworld";
  int fd = open("test_file", O_CREATE | O_RDWR);
  if(fd<0){
    exit();
  }
  if(write(fd, str, strlen(str))!=strlen(str)){
      close(fd);
      exit();
  }
  sync();
  close(fd);
  exit();
}
