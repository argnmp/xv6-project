#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

/* #define TARGET_FILE 16777216
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
  sync(); 
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
  //sync();
  close(fd);
  exit();
}
/* char buf[BSIZE] = {0,};
int main(int argc, char * argv[]){
  for(int i = 0; i<BSIZE; i++){
    buf[i] = 't';
  }
  int fd;
  char* file_names[20] = {
    "file1",
    "file2",
    "file3",
    "file4",
    "file5",
    "file6",
    "file7",
    "file8",
    "file9",
    "file10",
    "file11",
    "file12",
    "file13",
    "file14",
    "file15",
    "file16",
    "file17",
    "file18",
    "file19",
    "file20",
  };
  for(int i = 0; i<1; i++){
    fd = open(file_names[i], O_CREATE | O_RDWR);
    udbg("open: %d", fd);
    for(int i = 0; i<LOOP; i++){
      if(write(fd, buf, sizeof(buf))!=sizeof(buf)){
        udbg("write file failed"); 
        close(fd);
        exit();
      }
    }
    // int res = sync();  
    // udbg("flushed blocks: %d", res);
    close(fd);
  }
  exit();
} */
