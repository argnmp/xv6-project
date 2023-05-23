// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, DELAYED};

// store data for thread
struct thread{
  int tid;
  struct proc* main;
  struct proc* prev; // connects threads belongs to main
  struct proc* next; // connects threads belongs to main
  struct proc* pth; // used for joining threads
  void* retval;
};

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  uint ssz;                    // stack size

  /*
   * this flag is used to manually control exit and thread_exit sequence
   * used by exec
   * in exit() and thread_exit(), if this flag is set, just make state DELAYED and unset the flag and return
   * if this value is 0, no delayed_exit
   * delayed_exit is set to address of process
   */
  int delayed_exit;
  struct proc* delayed_exit_addr;

  /*
   * only valid values for main thread
   */
  uint sz_base;                // initial sz value 
  uint sz_limit;               // determines upper limit size of memory allocation to process, 0 means infinite
  
  char* thstack;               // Bottom of empty thread memory space stack 
  char* thstack_sp;            // current stack pointer of thstack
  char* thstack_fp;            // Top of empty thread memory space stack
  struct thread th;
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
