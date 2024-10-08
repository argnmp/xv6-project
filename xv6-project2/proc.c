#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

// global thread id
int nexttid = 1;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->ssz = 0;
  p->sz_limit = 0;
  // tid is unique across all threads
  p->th.tid = nexttid++;
  p->th.main = p;
  p->th.next = p;
  p->th.prev = p;
  p->delayed_exit = 0;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n, int* addr)
{
  uint sz;
  uint sz_limit;
  struct proc *curproc = myproc();

  /*
   * if curproc is not main thread,
   * change to main thread and grow memory size of main thread
   * ptable lock is needed
   */
  acquire(&ptable.lock);
  curproc = curproc->th.main;

  *addr = curproc->sz;

  sz = curproc->sz;
  sz_limit = curproc->sz_limit;
  // check if requested memory exceeds memory limit of process
  if(sz_limit!=0 && sz+n > sz_limit){
    // if requested size exceeds the memory limit, return -1
    release(&ptable.lock);
    return -1;
  }
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0){
      release(&ptable.lock);
      return -1;
    }
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0){
      release(&ptable.lock);
      return -1;

    }
  }

  /*
   * return to current thread
   */
  curproc->sz = sz;
  curproc = myproc();

  switchuvm(curproc);

  release(&ptable.lock);

  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  acquire(&ptable.lock);
  /*
   * if the thread that called fork is not a main thread, change to main thread
   * set curthread, which means the thread that called fork syscall
   */
  curproc = curproc->th.main;
  struct proc* curthread = myproc();

  /*
   * all thread for current process should not be copyed to the new process
   * so, first update thmem stackspace and then update
   */

  np->thstack = curproc->thstack; 
  np->thstack_sp = curproc->thstack_sp;
  np->thstack_fp = curproc->thstack_fp;

  // save all thread address spaces except for the thread that called fork
  struct proc* cursor = curthread->th.next; 
  struct proc* next_cursor;

  while(curthread!=cursor && cursor->state == RUNNABLE){
    if(cursor == curproc){
      cursor = cursor->th.next;
      continue;
    }
    next_cursor = cursor->th.next;
    // move thstack_sp of np, becaues forked process does not copy existing threads
    save_thmem(cursor, np);
    cursor = next_cursor;
  } 

  /*
   * curproc->thstack_sp, remains same, 
   * but np->thstack_sp differs by saving other thread stack spaces 
   */

  // have to copy entire process address space becasue of the sbrk space
  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curthread;
  *np->tf = *curthread->tf;

  /*
   * copy process info
   */
  np->ssz = curproc->ssz;
  np->sz_limit = curproc->sz_limit;
   

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;
  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;


  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  /*
   * this case is when exit() syscall is called from thread(not main thread)
   * stop the execution of current thread
   * and kill main thread(process)
   * later, main thread calls exit() and all other threads would be cleaned up
   */
  if((curproc != curproc->th.main) && (curproc->delayed_exit != 1)){
    acquire(&ptable.lock);
    curproc->state = ZOMBIE;
    curproc->th.main->killed = 1;
    wakeup1(curproc->th.main);
    sched();
    panic("zombie exit");
  }


  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      if(curproc->delayed_exit != 1){
        // close file if thread is not delayed exit state
        // which means that current sequence is normal process exit
        fileclose(curproc->ofile[fd]);
      }
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  /*
   * delayed exit sequence
   */
  if(curproc->delayed_exit == 1){
    curproc->state = DELAYED;
    curproc->delayed_exit = 0;
    // wakeup the thread that requested dealyed exit sequence of this thread
    wakeup1(curproc->delayed_exit_addr);
    sched();
    panic("zombie exit");
  }

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      /*
       * wait for process(main thread), not thread
       */
      if(p->state == ZOMBIE && p->th.main == p){
        /*
         * if target process has threads, remove them
         * this sequence is done by delayed exit
         */
        struct proc* cursor = p->th.next; 
        while(p!=cursor){
          if(cursor->state==UNUSED){
            cursor = cursor->th.next;
            continue;
          }
          if(cursor->state == RUNNING || cursor->state == RUNNABLE){
            /*
             * do delayed exit for RUNNING and RUNNABLE thread
             */
            delayed_exit(curproc, cursor);
          }
          // remove the resources of thread if it exited by delayed exit
          remove_th(cursor);
          cursor = cursor->th.next;
        } 
         
        // free process(main thread)
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->delayed_exit = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  int kill_succ = 0;
  struct proc *p;

  /*
   * set killed flag to main thread(process)
   * if main thread calls exit, it will remove threads
   */

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid && p->th.main == p){
      kill_succ = 1;
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING){
        p->state = RUNNABLE;
      }
    }
  }
  if(kill_succ == 1){
    release(&ptable.lock);
    return 0;
  }
  release(&ptable.lock);
  return -1;
}


/*
 * set memory allocation limit of a process
 */
int
setmemorylimit(int pid, int limit){
  struct proc *p;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    /*
     * setmemorylimmit target main thread(process)
     */
    if(p->pid == pid && p == p->th.main){
      if(limit == 0){
        // if limit == 0, set limit to infinite(sz_limit: 0)
        p->sz_limit = 0;
      }
      else {
        // check if the limit is smaller than pre-allocated memory
        if(limit < p->sz){
          release(&ptable.lock);
          return -1;
        }
        // set sz_limit to limit
        p->sz_limit = limit;
      }

      release(&ptable.lock);
      return 0;
    }
  }

  // pid doesn't exist. return -1
  release(&ptable.lock);
  return -1;
}

/*
 * get all process informations
 */
int
procinfo(struct proc_info_s* pinfos){
  struct proc *p;
  memset(pinfos->proc_arr, 0, sizeof(pinfos->proc_arr)); 
  pinfos->pcount = 0;

  acquire(&ptable.lock);

  // save process information to pinfos
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    // only targets main thread
    if(p != p->th.main){
      continue;
    }
    if(p->state == RUNNABLE || p->state == RUNNING || p->state == SLEEPING){
      pinfos->proc_arr[pinfos->pcount].pid = p->pid;
      safestrcpy(pinfos->proc_arr[pinfos->pcount].pname, p->name, sizeof(p->name));
      pinfos->proc_arr[pinfos->pcount].ssz = p->ssz;
      pinfos->proc_arr[pinfos->pcount].sz = p->sz;
      pinfos->proc_arr[pinfos->pcount].sz_limit = p->sz_limit;
      pinfos->pcount += 1;
    }
  }
  release(&ptable.lock);
  
  return 0;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [DELAYED]    "delayed"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


/*
 * These two functions are used for acquiring and releasing lock from outside of proc.c
 */
void ptable_lk_acquire(){
  acquire(&ptable.lock);
}
void ptable_lk_release(){
  release(&ptable.lock);
}
/*
 * These two functions are used for sleep and wakeup from outside of proc.c
 */
void wakeup1_wrapper(void* chan){
  wakeup1(chan);
}
void sleep_wrapper(void *chan){
  sleep(chan, &ptable.lock);
}



int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg){
  struct proc *np;
  struct proc *curproc = myproc();
  if((np = allocproc()) == 0){
    return -1;
  }
  uint sp, sz;

  acquire(&ptable.lock);
  /*
   * first check if it is possible to allocate memory for thread
   */
  if(curproc->th.main->sz_limit!=0 && curproc->th.main->sz +  2*PGSIZE > curproc->th.main->sz_limit){
    np->state = UNUSED;
    release(&ptable.lock);
    return -1;
  }
  /*
   * set basic thread values
   */
  np->th.main = curproc->th.main;
  np->pgdir = np->th.main->pgdir;
  np->pid = np->th.main->pid;
  np->parent = np->th.main->parent;

  /*
   * pid should remain the same
   */
  nextpid -= 1;
  np->pid = np->th.main->pid;

  /*
   * connect thread using linked list
   */
  np->th.next = curproc->th.next;
  curproc->th.next = np;
  np->th.prev = curproc;
  np->th.next->th.prev = np;
  np->th.pth = curproc;
  
  /*
   * copy trap frame from main thread
   * code from fork()
   */
  *np->tf = *(np->th.main->tf);

  /*
   * set eip of trapframe to start_routine
   * code from exec()
   */
  np->tf->eip = (uint)start_routine;

  /*
   * load stack memory space using load_thmem
   */
  sz = load_thmem(np);
  if(sz==0){
    np->state = UNUSED;
    np->th.prev->th.next = np->th.next;
    np->th.next->th.prev = np->th.prev;
    release(&ptable.lock);
    return -1;
  }
  /*
   * set fake return PC and arg pointer
   * code from exec()
   */
  sp = sz;
  sp -= 8;
  ((uint*)sp)[0] = 0xfffffff;
  ((uint*)sp)[1] = (uint)arg;
  
  /*
   * set stack pointer
   * code from exec()
   */
  np->tf->esp = sp;

  *thread = np->th.tid;

  np->sz = sz;
  np->ssz = 2*PGSIZE;
  np->sz_limit = sz;
  np->state = RUNNABLE;

  /*
   * share file descriptor of main
   */
  int i;
  for(i = 0; i < NOFILE; i++)
    if(np->th.main->ofile[i])
      np->ofile[i] = np->th.main->ofile[i];

  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, np->th.main->name, sizeof(np->th.main->name));

  release(&ptable.lock);
  return 0; 
}
void thread_exit(void *retval){
  struct proc *curproc = myproc();

  if(curproc == initproc)
    panic("init exiting");

  int fd;
  /*
   * thread does not free open files
   * only freed by main thread(process)
   */
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      curproc->ofile[fd] = 0;
    }
  }
  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  /*
   * used for delayed exit
   */
  if(curproc->delayed_exit == 1){
    curproc->state = DELAYED;
    curproc->delayed_exit = 0;
    wakeup1(curproc->delayed_exit_addr);
    sched();
    panic("zombie exit");
  }

  /*
   * wakeup parent thread
   */
  wakeup1(curproc->th.pth);

  curproc->state = ZOMBIE;
  curproc->th.retval = retval;
  sched();
  panic("thread zombie exit");
}
int thread_join(thread_t thread, void **retval){
  struct proc *p;
  int havethreads, havetargetthread;
  struct proc *curproc = myproc();
  acquire(&ptable.lock);
  /*
   * search by using main thread
   */
  for(;;){
    havethreads = 0;
    havetargetthread = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->th.main != curproc->th.main)
        continue;
      havethreads = 1;
      if(p->th.tid != thread)
        continue;
      havetargetthread = 1;
      if(p->state == ZOMBIE){
        /*
         * if target thread found, remove thread
         */
        *retval = p->th.retval;
        // update thread linkage
        p->th.prev->th.next = p->th.next;
        p->th.next->th.prev = p->th.prev;
        remove_th(p);
        release(&ptable.lock);
        return 0;
      }
    }
    if(!havethreads || !havetargetthread || curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    sleep(curproc, &ptable.lock);
  }
}
