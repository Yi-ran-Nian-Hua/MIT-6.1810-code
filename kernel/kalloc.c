// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages. 一共 4096 字节的页

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel. 内核所占空间过后的第一个字节, 作为用户内存的起点
                   // defined by kernel.ld.

struct run {
  struct run *next;
}; // 指向下一个节点的链表

struct {
  struct spinlock lock; // 自旋锁
  struct run *freelist; // 保存空闲节点的链表
} kmem; //

/**
 * 初始化函数
 */
void
kinit()
{
  initlock(&kmem.lock, "kmem"); // 首先初始化自旋锁
  freerange(end, (void*)PHYSTOP); // 设置好用户空闲内存的区间: [end, 0x80000000 + 128*1024*1024]
}

/**
 * 设置[@param pa_start, @param pa_end] 之间的区域为空闲内存
 * @param pa_start 开始点位
 * @param pa_end 结束点位
 */
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

/**
 * 分配一页物理内存, 返回内核可以使用的指针, 若内存无法分配则返回 0
 */
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void countFreeMem(uint64* count) {
  *count = 0;
  // 首先找到空闲列表的起始位置, 依次遍历即可
  struct run* pointer = kmem.freelist;

  acquire(&kmem.lock); // 加锁
  // 统计空闲内存大小
  while (pointer) {
    *count += PGSIZE; // 一页内存 4096 字节
    pointer = pointer->next;
  }
  release(&kmem.lock); // 解锁
}