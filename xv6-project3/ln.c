#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  /*
   * create link by specifying flags
   */
  if(argc != 4){
    printf(2, "Usage: ln [-h | -s] old new\n");
    exit();
  }
  if(strcmp(argv[1], "-h") == 0){
    if(link(argv[2], argv[3], 1) < 0)
      printf(2, "link %s %s: failed\n", argv[1], argv[2]);
  }
  else if(strcmp(argv[1], "-s") == 0){
    if(link(argv[2], argv[3], 2) < 0)
      printf(2, "link %s %s: failed\n", argv[1], argv[2]);
  }
  else {
    printf(2, "Usage: ln [-h | -s] old new\n");
  }
  sync();
  exit();
}
