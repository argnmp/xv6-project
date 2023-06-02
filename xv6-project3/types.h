typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

#define cdbg(fmt, args...) cprintf("[%d: %s] | " fmt "\n",__LINE__, __FUNCTION__, ##args)
#define udbg(fmt, args...) printf(1, "[%d: %s] pid %d | " fmt "\n",__LINE__, __FUNCTION__, getpid(), ##args)
