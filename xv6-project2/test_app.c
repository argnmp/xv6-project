#include "types.h"
#include "stat.h"
#include "user.h"
#define dbg(fmt, args...) printf(1, "[%d: %s] pid %d | " fmt "\n",__LINE__, __FUNCTION__, getpid(), ##args)
#define WORKER 9
#define WORKER2 4
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
//
// void* temp2(void* args){
//   int* ret = (int*)((int)200+(int)args);
//   // dbg("temp2");
//   //dbg("before thread_exit()");
//   thread_exit(ret);
//   return 0;
// }
// void* temp1(void* args){
//   int* ret = (int*)((int)200+(int)args);
//   // int dum = 1;
//   int pid = fork();
//   // dbg("temp1");
//
//   if(pid == 0){
//     // dbg("in th fork");
//
//     int res, retval = 0;
//     thread_t tids[WORKER2] = {0,};
//     //dbg("address of dum: %d, address of tids: %d\n", &dum, tids);
//     for(int i = 0; i<WORKER2; i++){
//       res = thread_create(&tids[i], temp2, (int*)i);
//       if(res==-1)
//         dbg("th create -> res: %d",res);
//     }
//     for(int i = 0; i<WORKER2; i++){
//       res = thread_join(tids[i], (void*)&retval);
//       if(res==-1)
//         dbg("th join -> tid: %d, res: %d, retval: %d", tids[i], res, retval);
//     }
//     //dbg("before exit()");
//     exit();
//   }
//   // dbg("after fork wait");
//   wait();
//   //dbg("before thread_exit()");
//   thread_exit(ret);
//   return 0;
// }
// thread_t tids[WORKER] = {0,};
// int main(int argc, char * argv[]){
//   int retval = 0;
//   // int res;
//   for(int i = 0; i<WORKER; i++){
//     /* res =  */thread_create(&tids[i], temp1, (int*)i);
//     // dbg("th create, res: %d", res);
//   }
//   for(int i = 0; i<WORKER; i++){
//     //dbg("1 / tid: %d, th join", tids[i]);
//     /* res =  */thread_join(tids[i], (void*)&retval);
//     //dbg("2 / tid: %d, th join", tids[i]);
//     //dbg("tid:%d, th join, res: %d, retval: %d",tids[i], res, retval);
//   }
//   //dbg("before exit()");
//   //for(;;);
//   exit();
// }
//
int main(int argc, char* argv[]){
  dbg("-------------------hello world-------------------\n");
  listcmd();
  exit();
}
