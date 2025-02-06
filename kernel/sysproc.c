#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "proc.h"
#include "fcntl.h"
#include "file.h"

extern int argfd(int n, int *pfd, struct file **pf);

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
	if (n < 0)
		n = 0;
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


uint64 sys_mmap(void) {
	// 首先获取参数
	uint64 addr;
	int length;
	int prot;
	int flags;
	int fd;
	struct file* file;
	int offset;
	uint64 err = 0xffffffffffffffff;

	argaddr(0, &addr);
	argint(1, &length);
	argint(2, &prot);
	argint(3, &flags);
	argfd(4,&fd, &file);
	argint(5, &offset);

	// 排除一些特殊情况
	if (addr != 0 || offset != 0 || length < 0) {
		printf("args err\n");
		return err;
	}


	// 如果文件不可写, 但是设置了 MAP_SHARED 表示要写回, 则返回错误
	if (file->writable == 0 && (prot & PROT_WRITE) != 0 && flags == MAP_SHARED) {
		printf("file not writable but flag is MAP_SHARED\n");
		return err;
	}

	struct proc* p = myproc(); // 获取当前进程信息
	// 查找当前进程的虚拟空间是否充足
	if (p->sz + length > MAXVA) {
		printf("vma not enough\n");
		return err;
	}

	// 开始查找没有使用的 VMA
	for (int i = 0; i < NVMA; i++) {
		if (p->vma[i].used == 0) {
			// 找到了没有使用的就进行映射
			p->vma[i].used = 1;
			p->vma[i].addr = p->sz;
			p->vma[i].length = length;
			p->vma[i].prot = prot;
			p->vma[i].fd = fd;
			p->vma[i].file = file;
			p->vma[i].offset = offset;
			p->vma[i].flags = flags;

			// 增加文件的引用计数, 以防止没有使用的时候消失
			filedup(file);
			p->sz += length;
			return p->vma[i].addr;
		}
	}
	printf("err!\n");
	return err;
}

uint64 sys_munmap(void) {
	// 获取参数
	uint64 addr;
	int length;
	argaddr(0, &addr);
	argint(1, &length);
	struct proc* p = myproc();
	// 遍历当前进程的 vma, 找到对应的虚拟内存页面取消映射即可
	int i;
	for (i = 0; i < NVMA; i++) {

		if (p->vma[i].used && p->vma[i].length >= length) {
			// 从起始位置开始寻找
			if (p->vma[i].addr == addr) {
				p->vma[i].addr += length;
				p->vma[i].length -= length;
				break;
			}
			// 从结束位置开始寻找
			if (p->vma[i].addr + p->vma[i].length == addr + length) {
				p->vma[i].length -= length;
				break;
			}
		}
	}

	if (i == NVMA) {
		// 没有找到对应的 vma
		return -1;
	}

	// 检查是否为 MAP_SHARED, 如果是的话还需要写回文件中
	if (p->vma[i].flags == MAP_SHARED && (p->vma[i].prot & PROT_WRITE) != 0) {
		filewrite(p->vma[i].file, addr, length);
	}

	// 取消与物理页面的映射关系
	uvmunmap(p->pagetable, addr, length / PGSIZE, 1);

	// 取消虚拟地址与文件的映射
	if (p->vma[i].length == 0) {
		fileclose(p->vma[i].file);
		p->vma[i].used = 0;
	}

	return 0;
}