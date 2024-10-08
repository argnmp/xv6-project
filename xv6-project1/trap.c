#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "procext.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
                        
struct spinlock tickslock;
uint ticks;

/*
 * newly defined ticks that records 100 ticks
 */
struct spinlock schedtickslock;
uint schedticks;

// procext.c
extern struct mlfq mlfq;
extern struct spinlock mlfq_lock;


void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  // test trap that can be called from user
  SETGATE(idt[T_USERINT], 1, SEG_KCODE<<3, vectors[T_USERINT], DPL_USER);
  SETGATE(idt[T_SCHEDULERLOCK], 1, SEG_KCODE<<3, vectors[T_SCHEDULERLOCK], DPL_USER);
  SETGATE(idt[T_SCHEDULERUNLOCK], 1, SEG_KCODE<<3, vectors[T_SCHEDULERUNLOCK], DPL_USER);

  initlock(&tickslock, "time");
  
  //+
  initlock(&schedtickslock, "schedtime");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;

    syscall();

    if(myproc()->killed)
      exit();
    return;
  }

  /*
   * interrupt 128, 129
   */
  
  if(tf->trapno == T_SCHEDULERLOCK){
    if(myproc()->killed)
      exit();
    //-cprintf("[trap()] schedulerLock interrupt called\n");
    schedulerLock(2019097210);
    if(myproc()->killed)
      exit();
    return;
  }
  if(tf->trapno == T_SCHEDULERUNLOCK){
    if(myproc()->killed)
      exit();
    //-cprintf("[trap()]schedulerUnlock interrupt called\n");
    schedulerUnlock(2019097210);
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;

      acquire(&schedtickslock);
      schedticks++;

      // decrease the left time quantum of running process before interrupt
      if(myproc()){
        if(myproc()->ticks > 0){
          myproc()->ticks -= 1;
        }
        //-cprintf("timer interrupt! running process pid: %d, decreased ticks: %d\n", myproc()->pid, myproc()->ticks);
      }

      // if schedticks reaches 100, execute priority boost
      if(schedticks >= 100){

        // acquire mlfq_lock for synchronization
        acquire(&mlfq_lock);
        
        // priority boost
        boost_mlfq();

        // if there is a process that was running holding scheduler lock,
        // unlock the scheduler lock
        if(myproc() && mlfq.locking_pid == myproc()->pid){
          //-cprintf("return to mlfq! pid: %d, level: %d, ticks: %d\n", myproc()->pid, myproc()->level, myproc()->ticks);

          // reset the ticks and move to the front of L0
          mlfq.locking_pid = 0;
          myproc()->ticks = L0_tq;

          remove_mlfq(myproc());
          push_mlfq_front(myproc(), L0, 3);

        }
        //-view_mlfq_status();

        release(&mlfq_lock);
        schedticks = 0;
      }
      release(&schedtickslock);

      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
      tf->trapno == T_IRQ0+IRQ_TIMER){

    
    if(MLFQ_SCH_SCHEME == 0){
      /*
       * specification implementation branch
       */
      acquire(&mlfq_lock);
      if(myproc()->ticks == 0){
        /*
         * reschedule the process if it used all of its allocated time quantum
         */

        //cprintf("pid: %d ticks over, reschedule!\n", myproc()->pid);
        reschedule_mlfq(myproc());
        //cprintf("\tafter_reschedule\n");
        //view_mlfq_status();
        //cprintf("\n");
      }
      else {
        if(myproc()->level == L2){
          /*
           * L2 queue should be served by FCFS
           * To prevent the case that waiting process in L2 queue is served later than the new process entered in L2, the process in L2 is resheduled to same queue every tick
           */

          //cprintf("pid: %d reschedule to last\n", myproc()->pid);
          reschedule_mlfq_to_last(myproc());
          //cprintf("\tafter_reschedule\n");
          //view_mlfq_status();
        }
      }
      release(&mlfq_lock);
      
      // yield if no process holds scheduler lock
      if(mlfq.locking_pid == 0){
        yield();
      }
    }
    else if(MLFQ_SCH_SCHEME == 1){
      /*
       * additional implemetation branch
       */
      acquire(&mlfq_lock);

      /*
       * this additional implementation holds the cpu until it used all of its time quantum
       */
      if(myproc()->ticks == 0 && mlfq.locking_pid==0){
        //-cprintf("pid: %d ticks over, schedule!\n", myproc()->pid);
        reschedule_mlfq(myproc());
        //-cprintf("\tafter_reschedule\n");
        //-view_mlfq_status();
        //-cprintf("\n");
        release(&mlfq_lock);

        yield();
      }
      else {
        release(&mlfq_lock);
      }

    }
    else {
      panic("MLFQ_SCH_SCHEME parameter is invalid");
    }
    
  }

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
