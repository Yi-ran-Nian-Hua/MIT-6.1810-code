#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_exit(void) {
	int n;
	argint(0, &n);
	exit(n);
	return 0;  // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
	uint64 p;
	argaddr(0, &p);
	return wait(p);
}

uint64 sys_sbrk(void) {
	uint64 addr;
	int n;

	argint(0, &n);
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1;
	return addr;
}

uint64 sys_sleep(void) {
	int n;
	uint ticks0;


	argint(0, &n);
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if (killed(myproc())) {
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}


#ifdef LAB_PGTBL
int sys_pgaccess(void) {
	// 获取调用的三个参数

	uint64 startAddr; // 第一个参数, 第一个检查的地址
	int pageNum; // 第二个参数, 要查找的页号
	unsigned int bitsMap = 0; // 第三个参数保存的位图, 稍后需要传给用户态
	uint64 userBitAddr;
	argaddr(0, &startAddr);
	argint(1, &pageNum);
	if (pageNum > 32)
		pageNum = 32;
	argaddr(2, &userBitAddr);
	for (int i = 0; i < pageNum; ++i) {
		pte_t* pte = walk(myproc()->pagetable, (uint64)startAddr + i * PGSIZE, 0);
		if (*pte & PTE_A) {
			*pte &= ~PTE_A;
			bitsMap |= (1 << i);
		}
	}

	// 最后将位图结果传给用户态
	if (copyout(myproc()->pagetable, userBitAddr,
		(char*)&bitsMap, sizeof(bitsMap)) < 0) {
		return -1;
	}

	return 0;
}
#endif

uint64 sys_kill(void) {
	int pid;

	argint(0, &pid);
	return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}
