typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

/*
 * proc_info structure
 */
struct proc_info_s{
  struct proc_i {
    int pid;
    char pname[16];
    uint ssz;
    uint sz;
    uint sz_limit;
  } proc_arr[64];
  int pcount;
};

/*
 * thread identifier
 */
typedef int thread_t;
