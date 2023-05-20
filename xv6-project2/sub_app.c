#include "types.h"
#include "stat.h"
#include "user.h"
#define dbg(fmt, args...) printf(1, "[%d: %s] pid %d | " fmt "\n",__LINE__, __FUNCTION__, getpid(), ##args)
#define WORKER1 10
#define WORKER2 5
int listcmd(){
  struct proc_info_s pinfos; 
  int res = procinfo(&pinfos);
  if(res < 0) return -1;  
  for(int i = 0; i < pinfos.pcount; i++){
    // printf(1, "pid: %d | tid: %d | number of stack pages: %d | allocated memory size: %d | memory limit %d\n", pinfos.proc_arr[i].pid, pinfos.proc_arr[i].tid, pinfos.proc_arr[i].ssz / PAGE_SIZE, pinfos.proc_arr[i].sz, pinfos.proc_arr[i].sz_limit); 
    printf(1, "pid: %d | tid: %d | ssz: %d | sz: %d | sz_limit %d | sz_base %d\n", pinfos.proc_arr[i].pid, pinfos.proc_arr[i].tid, pinfos.proc_arr[i].ssz, pinfos.proc_arr[i].sz, pinfos.proc_arr[i].sz_limit, pinfos.proc_arr[i].sz_base); 

  }
  return 0;
}
void* temp2(void* args){
  int* ret = 0;
  for(;;){
    dbg("hello world");
  }
  thread_exit(&ret);
  return 0;
}
void* temp1(void* args){
  //listcmd();
  int* ret;
  char* execargv[10]; 
  execargv[0] = "test_app";
  int res = exec(execargv[0], execargv);
  dbg("res: %d", res);
  thread_exit(&ret); 
  return 0;
}
int main(int argc, char * argv[]){
  int retval = 0;
  thread_t tids[WORKER1] = {0,};
  for(int i = 0; i<WORKER1; i++){
    int res = thread_create(&tids[i], temp2, (int*)i);
    if(res < 0)
      dbg("thc failed");
  }
  sleep(30);
  dbg("before exec");
  char* execargv[10]; 
  execargv[0] = "ls";
  int res = exec(execargv[0], execargv);
  dbg("res: %d", res);
  
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
  }
  exit();
}
