#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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

  argint(0, &pid);
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

uint64 sys_trace(void) {
  struct proc* myProc = myproc(); // 首先获取当前进程信息
  int maskNum; // 获取要追踪的系统调用号
  argint(0, &maskNum);
  myProc->traceMask = maskNum; // 将其保存到进程结构体中
  return 0;
}

uint64 sys_sysinfo(void) {
  struct sysinfo sysinfo;
  // 首先获取用户地址
  uint64 destAddr;
  argaddr(0, &destAddr);
  // 获取空闲内存量
  countFreeMem(&sysinfo.freemem);
  // 获取进程数
  getProc(&sysinfo.nproc);
  // 最后返回给用户态
  if(copyout(myproc()->pagetable, destAddr, (char *)&sysinfo, sizeof(sysinfo)) < 0)
    return -1;
  return 0;
}
