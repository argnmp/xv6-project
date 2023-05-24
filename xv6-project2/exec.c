#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();
  struct proc *cursor;

  /*
   * remove all threads except for current thread
   * This task requires ptable.lock
   * delayed exit is used for this sequence
   */

  ptable_lk_acquire();
    cursor = curproc->th.next; 
    while(cursor != curproc){
      // cprintf("await cursor pid: %d, tid: %d\n", cursor->pid, cursor->th.tid);

      if(cursor->state==UNUSED){
        cursor = cursor->th.next;
        continue;
      }

      if(cursor->state == RUNNING || cursor->state == RUNNABLE){
        // cprintf("running\n!");
        delayed_exit(curproc, cursor);
        if(cursor->state != DELAYED) panic("wrong!");
      }

      // cprintf("pid: %d, tid: %d, parent: %d, state: %d, killed: %d\n", cursor->pid, cursor->th.tid, cursor->parent->pid, cursor->state, cursor->killed);
      kfree(cursor->kstack);
      cursor->kstack = 0;
      cursor->state = UNUSED;
      cursor->pid = 0;
      cursor->parent = 0;
      cursor->name[0] = 0;
      cursor->killed = 0;
      cursor->delayed_exit = 0;

      cursor = cursor->th.next; 

    }
    //cprintf("end of while\n");

      // sleep_wrapper(curproc);
  curproc->th.main = curproc;
  curproc->th.next= curproc;
  curproc->th.prev = curproc;
  curproc->sz_limit = 0;
  ptable_lk_release();


  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  /*
   * allocate three pages
   * upper two pages are for stack
   * lower one page is used for saving deallocated thread memory space
   */
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 3*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;
  // set thread memory stack
  // thstack memory must not be accessible to user
  clearpteu(pgdir, (char*)(sz - 3*PGSIZE));
  curproc->thstack = (char*)(sz - 3*PGSIZE);
  curproc->thstack_sp = (char*)(sz - 2*PGSIZE);
  curproc->thstack_fp = (char*)(sz - 2*PGSIZE);

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  /*
   * need to reset thread data
   */
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->sz_base  = sz;
  curproc->ssz = 1*PGSIZE;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}

/*
 * exec2 implementation
 */
int
exec2(char *path, char **argv, int stacksize)
{
  /*
   * almost same as exec except allocating stack page
   */
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();
  struct proc *cursor;

  /*
   * remove all threads except for current thread
   * This task requires ptable.lock
   * delayed exit is used for this sequence
   */

  ptable_lk_acquire();
    cursor = curproc->th.next; 
    while(cursor != curproc){
      // cprintf("await cursor pid: %d, tid: %d\n", cursor->pid, cursor->th.tid);

      if(cursor->state==UNUSED){
        cursor = cursor->th.next;
        continue;
      }

      if(cursor->state == RUNNING || cursor->state == RUNNABLE){
        // cprintf("running\n!");
        delayed_exit(curproc, cursor);
        if(cursor->state != DELAYED) panic("wrong!");
      }

      // cprintf("pid: %d, tid: %d, parent: %d, state: %d, killed: %d\n", cursor->pid, cursor->th.tid, cursor->parent->pid, cursor->state, cursor->killed);
      kfree(cursor->kstack);
      cursor->kstack = 0;
      cursor->state = UNUSED;
      cursor->pid = 0;
      cursor->parent = 0;
      cursor->name[0] = 0;
      cursor->killed = 0;
      cursor->delayed_exit = 0;

      cursor = cursor->th.next; 

    }
    //cprintf("end of while\n");

      // sleep_wrapper(curproc);
  curproc->th.main = curproc;
  curproc->th.next= curproc;
  curproc->th.prev = curproc;
  curproc->sz_limit = 0;
  ptable_lk_release();

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.

  /*
   * allocate 'stacksize' number of pages
   * PGSIZE + stacksize * PGSIZE
   * one for guard page and others for stack
   * and one page is used for saving deallocated thread memory space
   */
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE + stacksize * PGSIZE)) == 0)
    goto bad;

  /*
   * make guard page inaccessible by clearing PTE_U
   */
  clearpteu(pgdir, (char*)(sz - (PGSIZE + stacksize * PGSIZE)));
  sp = sz;

  // set thread memory stack
  clearpteu(pgdir, (char*)(sz - (2*PGSIZE + stacksize * PGSIZE)));
  curproc->thstack = (char*)(sz - (2*PGSIZE + stacksize * PGSIZE));
  curproc->thstack_sp = (char*)(sz - (PGSIZE + stacksize * PGSIZE));
  curproc->thstack_fp = (char*)(sz - (PGSIZE + stacksize * PGSIZE));

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->sz_base = sz;
  curproc->ssz = stacksize*PGSIZE;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
