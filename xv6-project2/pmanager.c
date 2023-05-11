#include "types.h"
#include "user.h"
#include "fcntl.h"

#define PAGE_SIZE 4096

struct args_s{
  char* arg[3];
};

int get_input(char* buf, int buf_size){
  printf(1, "pmanager=# ");
  memset(buf, 0, buf_size);
  gets(buf, buf_size);
  if(buf[0] == 0)
    return -1;
  return 0;
}
int listcmd(){
  struct proc_info_s pinfos; 
  int res = procinfo(&pinfos);
  if(res < 0) return -1;  
  for(int i = 0; i < pinfos.pcount; i++){
    printf(1, "pid: %d | number of stack pages: %d | allocated memory size: %d | memory limit %d\n", pinfos.proc_arr[i].pid, pinfos.proc_arr[i].ssz / PAGE_SIZE, pinfos.proc_arr[i].sz, pinfos.proc_arr[i].sz_limit); 

  }
  return 0;
}
int killcmd(int pid){
  return kill(pid);
} 
int run(struct args_s* args){
  if(strcmp(args->arg[0], "list")==0){
    int res = listcmd(); 
    if(res < 0){
      printf(1, "list failed\n");
    }
  }
  else if(strcmp(args->arg[0], "kill")==0){
    int pid = atoi(args->arg[1]); 
    int res = kill(pid);
    if(res < 0){
      printf(1, "kill pid %d failed\n", pid);
    }
  }
  else if(strcmp(args->arg[0], "exit")==0){
    exit(); 
  } 
  return 0;
}

char stvrn[] = " \t\v\r\n";
char rn[] = "\r\n";
char stv[] = " \t\v";
int main(){
  static char buf[100];     
  while(get_input(buf, sizeof(buf)) == 0){
    // parse command
    char* pstr = buf;
    char* str = buf;
    struct args_s args;
    args.arg[0] = "\0";
    args.arg[1] = "\0";
    args.arg[2] = "\0";

    for(int count = 0; count <=2; count+=1){
      for(;strchr(stv, *str)!=0; str+=1);
      pstr = str;
      for(;strchr(stvrn, *str)==0; str+=1);
      args.arg[count] = pstr;
      if(strchr(rn, *str)!=0){
        *str = '\0';
        break;
      }
      else {
        *str = '\0';
      }
      str+=1;
      pstr = str;
    }
    
    run(&args);
  }
  exit(); 
}
