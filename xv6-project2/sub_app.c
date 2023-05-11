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
  printf(1, "This is sub program\n");
  for(int i = 0; i<10; i++){
    int res = setmemorylimit(getpid(), 16384 + i);
    printf(1, "malloc setmemorylimit res: %d\n", res);
  }
  exit();
}
