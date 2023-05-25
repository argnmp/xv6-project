#include "types.h"
#include "stat.h"
#include "user.h"
#define dbg(fmt, args...) printf(1, "[%d: %s] pid %d | " fmt "\n",__LINE__, __FUNCTION__, getpid(), ##args)
#define WORKER1 20
#define WORKER2 6
int listcmd(){
  struct proc_info_s pinfos; 
  int res = procinfo(&pinfos);
  if(res < 0) return -1;  
  for(int i = 0; i < pinfos.pcount; i++){
    // printf(1, "pid: %d | tid: %d | number of stack pages: %d | allocated memory size: %d | memory limit %d\n", pinfos.proc_arr[i].pid, pinfos.proc_arr[i].tid, pinfos.proc_arr[i].ssz / PAGE_SIZE, pinfos.proc_arr[i].sz, pinfos.proc_arr[i].sz_limit); 
    printf(1, "pid: %d | tid: %d | ssz: %d | sz: %d | sz_limit %d\n", pinfos.proc_arr[i].pid, pinfos.proc_arr[i].tid, pinfos.proc_arr[i].ssz, pinfos.proc_arr[i].sz, pinfos.proc_arr[i].sz_limit); 

  }
  return 0;
}
/*
 * sbrk test
 */

/* struct task{
  int* addr; 
  int idx;
};
void* job(void* args){
  int* res = (int*)sbrk(sizeof(int));
  struct task* t = args;
  int sum = 0;
  // add given values
  for(int i = 0; i<10; i++){
    sum += t->addr[i];
  }
  // add signature value
  sum += t->idx * 100000;
  *res = sum;
  thread_exit(res);
  return 0;
}
int main(int argc, char * argv[]){
  thread_t tids[WORKER1] = {0,};
  int* buf = (int*)sbrk(sizeof(int)*10);
  for(int i = 0; i<10; i++){
    buf[i] = 70+i;
  }
  for(int i = 0; i<WORKER1; i++){
    struct task* t = (struct task*)sbrk(sizeof(struct task));
    t->addr = buf;
    t->idx = i;
    int res = thread_create(&tids[i], job, t);
    if(res < 0)
      dbg("thread create failed");
  }
  int* retval;
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
    dbg("join th %d, res: %d", tids[i], *retval);
  }
  
  exit();
} */

/*
 * exec test
 */
void* job(void* args){
  if((int)args == 5){
    char* execargv[10]; 
    execargv[0] = "test_app";
    int res = exec(execargv[0], execargv);
    if(res < 0){
      dbg("exec failed\n");
    }
  }
  thread_exit(0);
  return 0;
}
int main(int argc, char* argv[]){
  thread_t tids[WORKER1] = {0,}; 
  for(int i = 0; i<WORKER1; i++){
    thread_create(&tids[i], job, (void*)i);
  }
  int* retval;
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
  }
  exit(); 
}

/*
 * exit test
 */
/* void* job(void* args){
  for(;;);
  thread_exit(0);
  return 0;
}
int main(int argc, char* argv[]){
  thread_t tids[WORKER1] = {0,}; 
  for(int i = 0; i<WORKER1; i++){
    thread_create(&tids[i], job, (void*)i);
  }
  int* retval;
  dbg("sleep main");
  sleep(20);
  dbg("exit!");
  exit(); 
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
  }
} */

/*
 * fork test
 */
/* void* subtask(void* args){
  int* ret = (int*)((int)700+(int)args);
  thread_exit(ret);
  return 0;
}
void* task(void* args){
  int* ret = (int*)((int)200+(int)args);

  int pid = fork();
  if(pid == 0){

    int retval = 0;
    thread_t tids[WORKER2] = {0,};
    for(int i = 0; i<WORKER2; i++){
      thread_create(&tids[i], subtask, (int*)i);
      thread_join(tids[i], (void*)&retval);
      dbg("tid %d -> subtask result: %d", tids[i], (int)retval);
    }
    exit();
  }
  wait();
  dbg("subtask finished");
  thread_exit(ret);
  return 0;
}
int main(int argc, char * argv[]){
  thread_t tids[WORKER1] = {0,};
  int retval = 0;
  for(int i = 0; i<WORKER1; i++){
    thread_create(&tids[i], task, (void*)i);
    thread_join(tids[i], (void*)&retval);
    dbg("tid %d -> task result: %d", tids[i], (int)retval);
    printf(1, "\n");
  }
  exit();
} */
/*
 * kill test
 */
/* void* job(void* args){
  if((int)args == 4){
    dbg("sleep start");
    sleep(100);
    dbg("sleep end");
    kill(getpid());
  }
  sleep(100000);
  thread_exit(0);
  return 0;
}
int main(int argc, char* argv[]){
  thread_t tids[WORKER1] = {0,}; 
  for(int i = 0; i<WORKER1; i++){
    thread_create(&tids[i], job, (void*)i);
  }
  int* retval;
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
  }
  exit(); 
} */
/*
 * setmemlimit with thread_create test
 */
/* void* job(void* args){
  if((int)args == 5){
    setmemorylimit(getpid(), 98304);
  } 
  thread_exit(0);
  return 0;
}
int main(int argc, char* argv[]){
  int* retval;
  thread_t tids[WORKER1] = {0,}; 
  for(int i = 0; i<WORKER1; i++){
    int res = thread_create(&tids[i], job, (void*)i);
    dbg("th create res: %d", res); 
  }
  for(int i = 0; i<WORKER1; i++){
    thread_join(tids[i], (void*)&retval);
  }
  sleep(10000);
  exit();
} */
