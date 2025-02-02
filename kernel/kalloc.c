// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"


#define PA2PGREF_ID(p) (((p)-KERNBASE)/PGSIZE) //  通过物理地址获取物理页id
#define PGREF_MAX_ENTRIES PA2PGREF_ID(PHYSTOP) //  获取物理页的上限


int pageRef[PGREF_MAX_ENTRIES]; // 该数组表示每个物理页的引用数, pageRef[i] 表示第 i 个的引用数
struct spinlock refLock; // 控制引用数的自旋锁
#define PA2PGREF(p) pageRef[PA2PGREF_ID((uint64)(p))] // 获取地址对应物理页的引用数

void freerange(void *pa_start, void *pa_end);

extern char end[];	// first address after kernel.
					// defined by kernel.ld.

struct run {
	struct run *next;
};

struct {
	struct spinlock lock;
	struct run *freelist;
} kmem;

void kinit() {
	initlock(&kmem.lock, "kmem");
	initlock(&refLock, "refLock");
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
	struct run *r = 0;

	if (((uint64) pa % PGSIZE) != 0 || (char *) pa < end || (uint64) pa >= PHYSTOP)
		panic("kfree");

	acquire(&refLock);
	if (--PA2PGREF(r) <= 0) {
		// Fill with junk to catch dangling refs.
		memset(pa, 1, PGSIZE);

		r = (struct run *) pa;

		acquire(&kmem.lock);
		r->next = kmem.freelist;
		kmem.freelist = r;
		release(&kmem.lock);
	}
	release(&refLock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
	struct run *r;

	acquire(&kmem.lock);
	r = kmem.freelist;
	if (r)
		kmem.freelist = r->next;
	release(&kmem.lock);

	if (r) {
		memset((char *) r, 5, PGSIZE);	// fill with junk
		PA2PGREF(r) = 1; // 将新分配的物理页的引用数设置为 1
	}


	return (void *) r;
}


/**
 * 增加对应物理地址的引用数
 * @param physicalAddr 物理地址
 */
void krefpage(void* physicalAddr) {
	acquire(&refLock);
	PA2PGREF(physicalAddr)++;
	release(&refLock);
}

/**
 * 写时复制的时候, 根据原物理地址分配一个新的物理页并返回
 * @param physicalAddr
 */
void* mykalloc(void* physicalAddr) {
	acquire(&refLock);

	// 如果当前物理页的引用计数为 1, 则无需分配新的物理页
	if (PA2PGREF(physicalAddr) <= 1) {
		release(&refLock);
		return physicalAddr;
	}

	// 否则就分配一个新的物理页, 复制数据, 将原始物理页的引用计数 -1
	uint64 newPage = (uint64)kalloc();
	if (newPage == 0) {
		release(&refLock);
		panic("mykalloc: alloc new page");
	}
	memmove((void*)newPage, physicalAddr, PGSIZE);
	PA2PGREF(physicalAddr)--;
	release(&refLock);

	return (void*)newPage;

}