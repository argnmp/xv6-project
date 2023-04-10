#define L0 0
#define L1 1
#define L2 2
#define L0_NPROC 64
#define L1_NPROC 64
#define L2_NPROC 64
#define L0_tq 4
#define L1_tq 6
#define L2_tq 8
#define NPRIORITY 4
#define PNONE -1
#define P0 0
#define P1 1
#define P2 2
#define P3 3

struct mlfq{
  // 0 for not locked | 1 for locked
  int locking_pid;

  int l0_cur;
  int l0_count;
  struct proc* l0[L0_NPROC];
  int l1_cur;
  int l1_count;
  struct proc* l1[L1_NPROC];
  // l2_cur and l2_count is deprecated
  int l2_count;
  int l2_cur[NPRIORITY];
  struct proc* l2[L2_NPROC];
};

// procext.c
int count_runnable_mlfq(int target);

// this function just push the process at the empty index started from the cursor in the target queue. So all the other information should have been updated.
int push_mlfq(struct proc* p, int target, int priority);

// all the other process information must have been updated before calling this code, need to add error case
// L2 is not supported
int push_mlfq_front(struct proc* p, int target, int priority);

int remove_mlfq(struct proc* p);

// priority is only used when target is L2
struct proc** get_mlfq_cur(int target, int priority);
struct proc** get_mlfq_cur_limit(int target, int priority);
struct proc** get_mlfq_cur_next(struct proc** cur, int target);
struct proc** move_mlfq_cur(int target, int priority);

// do we need lock for these functions?
// move process from queue to queue
int reset_mlfq_tq(struct proc* p);
int update_mlfq(struct proc* p, int target, int priority);
int reschedule_mlfq(struct proc* p);
int boost_mlfq();

// system call
int getLevel(void);
int setPriority(int pid, int priority);
int schedulerLock(int password);
int schedulerUnlock(int password);
