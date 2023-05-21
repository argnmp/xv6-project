#include "types.h"
#include "stat.h"
#include "user.h"
#define dbg(fmt, args...) printf(1, "[%s: %d] pid %d | " fmt "\n",__FUNCTION__, __LINE__, getpid(), ##args)
#define WORKER1 1
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
struct task{
  int* addr; 
  int idx;
};
void* temp3(void* args){
  dbg("%d", (int)args);
  thread_exit((void*)888); 
  return 0;
}
void* temp2(void* args){
  int tid;
  int* retval;
  struct task* t = args;

  thread_create(&tid, temp3, (int*)(t->idx));

  int res = thread_join(tid, (void*)&retval);
  dbg("res: %d, result: %d", res, retval);
  for(;;);

  for(int i = 0; i<10000; i++){
    t->addr[t->idx] += t->idx;
  }
  //sleep(10000);
  thread_exit((int*)777);
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
  thread_t tids[WORKER1] = {0,};
  int* buf = (int*)sbrk(sizeof(int)*10);
  if((int)buf == -1){
    dbg("sbrk failed");
    exit();
  }
  
  for(int i = 0; i<WORKER1; i++){
    struct task* t = (struct task*)sbrk(sizeof(struct task));
    if((int)t == -1){
      dbg("sbrk failed");
      exit();
    }
    t->addr = buf;
    t->idx = i;
    int res = thread_create(&tids[i], temp2, t);
    if(res < 0)
      dbg("thc failed");
  }
  for(;;){
    sleep(10);
    dbg("__________________hello world_______________ from main");
  }
  int* retval;
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
    dbg("join th %d, retval %d", tids[i], retval);
  }
  // for(int i = 0; i<10; i++){
  //   dbg("%d", buf[i]);
  // }
  
  exit();
}
