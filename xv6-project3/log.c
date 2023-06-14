#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
uint inodestart = 0;
struct logheader {
  int n;
  int block[LOGSIZE];
};

// contains the information of log creator which the same index to lh.block
// synchronization is done exactly the same way the existing log code synchronizes
int logpid[LOGSIZE];

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding; // how many FS sys calls are executing.
  int committing;  // in commit(), please wait.
  int dev;
  struct logheader lh;
};
struct log log;

/*
 * functions that are used for accessing log outside of log.c
 */
int getinodestart(){
  // deprecated
  return inodestart;
}
int* lhblockptr(){
  return log.lh.block;
}
int* lhnptr(){
  return &log.lh.n;
}
int* lhpidptr(){
  return logpid;
}
void acquireloglk(){
  acquire(&log.lock);
}
void releaseloglk(){
  release(&log.lock);
}

static void recover_from_log(void);
static void commit();

void
initlog(int dev)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  initlock(&log.lock, "log");
  readsb(dev, &sb);
  log.start = sb.logstart;
  log.size = sb.nlog;
  log.dev = dev;
  // deprecated
  inodestart = sb.inodestart;
  recover_from_log();
}

// Copy committed blocks from log to their home location
static void
install_trans(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(void)
{
  read_head();
  install_trans(); // if committed, copy from log to disk
  log.lh.n = 0;
  write_head(); // clear the log
}

// called at the start of each FS system call.
void
begin_op(void)
{
  acquire(&log.lock);
  while(1){
    if(log.committing){
      sleep(&log, &log.lock);
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      
      /*
       * if sync() is not called, buffer and log can be overflowed
       * call sync to create buffer and log space
       */
      // cdbg("begin op sync");

      release(&log.lock);
      ksync(0);
      acquire(&log.lock);

      continue;
    } else {
      log.outstanding += 1;
      release(&log.lock);
      break;
    }
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
  /*
   * seperate commit from end_op to sync
   */

  acquire(&log.lock);
  log.outstanding -= 1;
  if(log.committing)
    panic("log.committing");
  if(log.outstanding == 0){

  } else {
    // begin_op() may be waiting for log space,
    // and decrementing log.outstanding has decreased
    // the amount of reserved space.
    wakeup(&log);
  }
  release(&log.lock);
}

// Copy modified blocks from cache to log.
static void
write_log(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start+tail+1); // log block
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
    
    /*
     * set unsynchronized flag of buffer to 0
     * because now this buffer of block is synchronized to disk by calling sync()
     */
    from->unsynchronized = 0;
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelse(from);
    brelse(to);
  }
}

static void
commit()
{
  if (log.lh.n > 0) {
    write_log();     // Write modified blocks from cache to log
    write_head();    // Write header to disk -- the real commit
    install_trans(); // Now install writes to home locations
    log.lh.n = 0;
    write_head();    // Erase the transaction from the log
  }
}
int
sync(){
  /*
   * commit the logs
   */
  acquire(&log.lock);
  if(log.committing)
    panic("sync: commit should be occured here");

  if(log.outstanding != 0){
    wakeup(&log);
    release(&log.lock);
    return -1;
  } 
  log.committing = 1; 

  // flush_count is the same as the pending logs number
  int flush_count = log.lh.n;
  release(&log.lock);
  
  commit();

  acquire(&log.lock);
  log.committing = 0;
  wakeup(&log);
  release(&log.lock);
  return flush_count;
}
int
ksync(struct buf *b){
  /*
   * this function is used to commit() in the kernel code
   */
  /*
   * if b is not 0, release sleep lock -> commit -> acquire sleeplock
   */
  acquire(&log.lock);
  log.committing = 1;
  release(&log.lock);

  if(b!=0)
    releasesleep(&b->lock);

  commit(); 

  if(b!=0)
    acquiresleep(&b->lock);

  acquire(&log.lock);
  log.committing = 0;
  release(&log.lock);
  return 0;
}


// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache with B_DIRTY.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;

  while (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1){
    /*
     * log overflow case
     * call ksync(b) to reclaim buffer and log space
     */
    ksync(b);
    //panic("too big a transaction");
  }
  if (log.outstanding < 1)
    panic("log_write outside of trans");

  acquire(&log.lock);
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // log absorbtion
      break;
  }

  log.lh.block[i] = b->blockno;
  // write the pid of process that created this log
  logpid[i] = mypid();
  if (i == log.lh.n){
    log.lh.n++;
  }
  b->flags |= B_DIRTY; // prevent eviction
  /*
   * set the unsynchronized flag of buffer of written block to indicate
   * that this block is not yet synchornized to disk (which is done by sync())
   * unsynchronized is reset to 0 in write_log() that is called by commit() - sync()
   */
  b->unsynchronized = 1;
  release(&log.lock);
}

