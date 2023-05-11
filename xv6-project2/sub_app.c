#include "types.h"
#include "stat.h"
#include "user.h"
/* void recall(int n){
  char a[1<<10];
  memset(a, -1, sizeof(a));
  printf(1, "%d / %d\n", a[0], n);
  recall(n+1);
} */

int main(int argc, char * argv[]){
  struct proc_info_s pinfos;
  int res = procinfo(&pinfos);
  if(res == 0){
    for(int i = 0; i<pinfos.pcount; i++){
      printf(1, "pid: %d | ssz: %d | sz: %d | sz_limit: %d \n", pinfos.proc_arr[i].pid, pinfos.proc_arr[i].ssz, pinfos.proc_arr[i].sz, pinfos.proc_arr[i].sz_limit);
    } 
  }
  for(;;);
  exit();
}
