#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_mmap(void)
{
  uint64 addr; // always zero
  int length;
  int prot;
  int flags;
  int fd;
  int offset; // always zero
  struct proc *p = myproc();

  if (argaddr(0, &addr) || argint(1, &length) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0 || argint(4, &fd) < 0 || argint(5, &offset) < 0)
    return -1;

  // check the invalid prot considering fd's permission
  if ((prot & PROT_WRITE) && (flags & MAP_SHARED) && (!p->ofile[fd]->writable))
    return -1;

  // check if there is unused entry in the process' vma
  int vmaindex = -1;
  for (int i = 0; i < VMA_SIZE; ++i) {
    if (!p->vma[i].inuse) {
      vmaindex = i;
      p->vma[i].inuse = 1;
      break;
    }
  }
  if (vmaindex == -1)
    return -1;

  // find the unused region in the process' address space to map the file
  uint64 procbreak = p->sz;
  p->sz += length;

  // Write this into process' vma.
  p->vma[vmaindex].addr = procbreak;
  p->vma[vmaindex].size = length;
  p->vma[vmaindex].perm = prot;
  p->vma[vmaindex].fileoff = offset;
  p->vma[vmaindex].flag = flags;
  p->vma[vmaindex].mappedfile = p->ofile[fd];
  filedup(p->ofile[fd]);

  return procbreak;
}

uint64
sys_munmap(void)
{
  uint64 addr;
  int length;
  struct proc *p = myproc();

  if (argaddr(0, &addr) || argint(1, &length) < 0)
    return -1;

  // addr must be a multiple of the page size
  if (addr != PGROUNDDOWN(addr))
    return -1;

  // find the vma which addr belongs to
  struct VMA *vma = (struct VMA*)0;
  for (int i = 0; i < VMA_SIZE; ++i) {
    if (p->vma[i].inuse && p->vma[i].addr <= addr && addr < p->vma[i].addr + p->vma[i].size) {
      vma = &p->vma[i];
      break;
    }
  }
  if (vma == (struct VMA*)0)
    return -1;

  // split vma into 3 parts: before, tbd(to be deleted), after
  struct VMA before, tbd, after;
  before.inuse = 0;
  after.inuse = 0;
  {
    tbd = *vma;
    tbd.addr = addr;
    tbd.size = PGROUNDUP(length);
    tbd.fileoff += (addr - vma->addr);
    filedup(tbd.mappedfile);
  }
  if (addr > vma->addr) {
    // construct before
    before = *vma;
    before.size = addr - vma->addr;
    filedup(before.mappedfile);
  }
  if (addr + length < vma->addr + vma->size) {
    // construct after
    after = *vma;
    after.addr = PGROUNDUP(addr + length);
    after.size = (vma->addr + vma->size) - after.addr;
    after.fileoff = tbd.fileoff + (after.addr - tbd.addr);
    filedup(after.mappedfile);
  }

  // remove vma from p->vma
  int ret = munmap(&tbd);

  // insert before and after into p->vma
  vma->inuse = 0;
  fileclose(vma->mappedfile);
  if (before.inuse) {
    int success = 0;
    for (int i = 0; i < VMA_SIZE; ++i) {
      if (!p->vma[i].inuse) {
        p->vma[i] = before;
        success = 1;
        break;
      }
    }
    if (!success) return -1;
  }
  if (after.inuse) {
    int success = 0;
    for (int i = 0; i < VMA_SIZE; ++i) {
      if (!p->vma[i].inuse) {
        p->vma[i] = after;
        success = 1;
        break;
      }
    }
    if (!success) return -1;
  }

  return ret;
}
