// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[];	// first address after kernel.
					// defined by kernel.ld.

struct run {
	struct run *next;
};

struct {
	struct spinlock lock;
	struct run *freelist;
} kmem[NCPU]; // 给每个 CPU 都设置一个空闲列表还有自旋锁, 降低竞争
char* memoryLockName[] = {
	"kmem_cpu_0",
	"kmem_cpu_1",
	"kmem_cpu_2",
	"kmem_cpu_3",
	"kmem_cpu_4",
	"kmem_cpu_5",
	"kmem_cpu_6",
	"kmem_cpu_7",
};


void kinit() {
	for (int i = 0; i < NCPU; ++i) {
		initlock(&kmem[i].lock, memoryLockName[i]);
	}
	freerange(end, (void *) PHYSTOP);
}

void freerange(void *pa_start, void *pa_end) {
	char *p;
	p = (char *) PGROUNDUP((uint64) pa_start);
	for (; p + PGSIZE <= (char *) pa_end; p += PGSIZE) kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
	struct run *r;

	if (((uint64) pa % PGSIZE) != 0 || (char *) pa < end || (uint64) pa >= PHYSTOP)
		panic("kfree");

	// Fill with junk to catch dangling refs.
	memset(pa, 1, PGSIZE);

	r = (struct run *) pa;

	push_off(); // 关闭中断
	int cpuNum = cpuid(); // 获取调用 kfree 的 CPU 编号
	acquire(&kmem[cpuNum].lock);
	r->next = kmem[cpuNum].freelist;
	kmem[cpuNum].freelist = r;
	release(&kmem[cpuNum].lock);
	pop_off(); // 打开中断
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
	struct run *r;

	push_off(); // 获取 CPU id 之前需要关闭中断
	int cpuNum = cpuid();
	acquire(&kmem[cpuNum].lock);
	r = kmem[cpuNum].freelist;
	if (!r) {
		// 当自己 CPU 的 freelist 不足, 需要去其他 CPU 中偷一些 freelist 来
		int pageNum = 32; // 假设一次偷 32 个 page
		for (int i = 0; i < NCPU; ++i) {
			if (i == cpuNum)
				continue;
			acquire(&kmem[i].lock);
			if (!kmem[i].freelist) {
				// 如果遍历到的这一个 CPU 也没有空闲页了, 直接看下一个
				release(&kmem[i].lock);
				continue;
			}
			struct run* temp = kmem[i].freelist;
			while (temp && pageNum > 0) {
				// 循环将另外 CPU 的页给复制过来
				// 取下一页空白页
				kmem[i].freelist = temp->next;
				// 之后接到自己 CPU 上
				temp->next = kmem[cpuNum].freelist;
				kmem[cpuNum].freelist = temp;
				temp = kmem[i].freelist; // 继续回到开头
				pageNum--;
			}
			release(&kmem[i].lock);
			if (pageNum == 0)
				break;
		}
	}

	r = kmem[cpuNum].freelist;
	if (r) {
		kmem[cpuNum].freelist = r->next;// 取出一个空闲页
	}
	release(&kmem[cpuNum].lock); // 释放锁
	pop_off(); // 打开中断

	if (r)
		memset((char *) r, 5, PGSIZE);	// fill with junk
	return (void *) r;
}
