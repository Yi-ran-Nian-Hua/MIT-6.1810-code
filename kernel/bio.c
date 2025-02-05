// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKETNUM 13
#define HASH(x) (x % BUCKETNUM)

struct bucket{
	struct buf head;
	struct spinlock bucketLock;
};

struct {
	struct buf buf[NBUF];
	struct bucket buckets[BUCKETNUM];
} bcache;

char* bucketName[]={
	"bcache_0", "bcache_1", "bcache_2", "bcache_3",
	"bcache_4", "bcache_5", "bcache_6", "bcache_7",
	"bcache_8", "bcache_9", "bcache_10", "bcache_11",
	"bcache_12"
};

void binit(void) {
	struct buf *b;
	for (int i = 0; i < 13; i++) {
		// 初始化锁
		initlock(&bcache.buckets[i].bucketLock, bucketName[i]);
		// 初始化链表的节点, 指向自身
		bcache.buckets[i].head.prev = &bcache.buckets[i].head;
		bcache.buckets[i].head.next = &bcache.buckets[i].head;
	}

	// Create linked list of buffers
	for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
		b->next = bcache.buckets[0].head.next;
		b->prev = &bcache.buckets[0].head;
		initsleeplock(&b->lock, "buffer");
		bcache.buckets[0].head.next->prev = b;
		bcache.buckets[0].head.next = b;
	}

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *bget(uint dev, uint blockno) {
    struct buf *b;

    int hash = HASH(blockno);

    acquire(&bcache.buckets[hash].bucketLock);

    // Is the block already cached?
    for (b = bcache.buckets[hash].head.next; b != &bcache.buckets[hash].head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;

            // 更新时间戳
            acquire(&tickslock);
            b->timeStamp = ticks;
            release(&tickslock);

        	release(&bcache.buckets[hash].bucketLock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    b = 0; // 清空 b, 后面有用
    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    // LRU 算法: 首先寻找当前的桶是否有没有引用并且时间戳最小的块, 如果有直接返回
    // 如果没有, 就遍历下一个桶, 遍历下一个桶之前释放当前桶的锁获取下一个桶的锁
    // 找到之后将该块移动到自己的桶中来
    int loopCount = 0; // 寻找桶的次数
	struct buf *temp;

    for (int bucketNum = hash; loopCount != BUCKETNUM;
        bucketNum = (bucketNum + 1) % BUCKETNUM ) {
    	loopCount++;
        // 自己的桶不用重新获取锁, 因为锁没有释放
        if (bucketNum != hash) {
            // 检查是否有锁
            if (!holding(&bcache.buckets[bucketNum].bucketLock)) {
                acquire(&bcache.buckets[bucketNum].bucketLock); // 如果没有锁就获取锁
            }else {
                continue; // 获取锁就直接看下一个桶
            }
        }
        // 之后遍历 buffer, 找到没有引用并且时间戳最小的块
        for (temp = bcache.buckets[bucketNum].head.next; temp != &bcache.buckets[bucketNum].head;
            temp = temp->next) {
            if (temp->refcnt == 0 && (b == 0 || temp->timeStamp < b->timeStamp)) {
                b = temp;
            }
        }

        // 表示找到了这个块
        if (b) {
            // 判断这个块是从哪个 bucket 中获取的
            if (bucketNum != hash) {
                // 不是从自己的 bucket 中获取的, 就需要插入到自己的 bucket 中
                // 首先从原来的 bucket 中的链表中删除
                b->next->prev = b->prev;
                b->prev->next = b->next;
                release(&bcache.buckets[bucketNum].bucketLock);

                // 之后插入到自己的链表中, 插入到头部
                b->next = bcache.buckets[hash].head.next;
                b->prev = &bcache.buckets[hash].head;
                bcache.buckets[hash].head.next->prev = b;
                bcache.buckets[hash].head.next = b;

            }
            // 之后释放掉相对应的锁即可
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;

            acquire(&tickslock);
            b->timeStamp = ticks;
            release(&tickslock);

            release(&bcache.buckets[hash].bucketLock);    // 释放掉自己的锁
            acquiresleep(&b->lock);
            return b;
        }else{
            // 没有找到这个块, 则表示当前的 bucket 中没有, 继续去找下一个 bucket
            // 释放掉当前 bucket 的锁
    		if (bucketNum != hash)
				release(&bcache.buckets[bucketNum].bucketLock);
        }
    }
    panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *bread(uint dev, uint blockno) {
	struct buf *b;

	b = bget(dev, blockno);
	if (!b->valid) {
		virtio_disk_rw(b, 0);
		b->valid = 1;
	}
	return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) {
	if (!holdingsleep(&b->lock))
		panic("bwrite");
	virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b) {
	if (!holdingsleep(&b->lock))
		panic("brelse");

	releasesleep(&b->lock);

	int hash = HASH(b->blockno); // 根据 blockno 获取对应的 bucket 号
	acquire(&bcache.buckets[hash].bucketLock);
	b->refcnt--;
	/*if (b->refcnt == 0) {
		// 当引用计数为 0 的时候, 可以直接剔除出当前 bucket
		// no one is waiting for it.
		b->next->prev = b->prev;
		b->prev->next = b->next;
		b->next = bcache.buckets[hash].head.next;
		b->prev = &bcache.buckets[hash].head;
		bcache.buckets[hash].head.next->prev = b;
		bcache.buckets[hash].head.next = b;
	}else {*/
		// 引用计数不为 0 的时候, 更新时间戳
		acquire(&tickslock);
		b->timeStamp = ticks; // 更新时间戳
		release(&tickslock);
	//}

	release(&bcache.buckets[hash].bucketLock);
}

void bpin(struct buf *b) {

	acquire(&bcache.buckets[HASH(b->blockno)].bucketLock);
	b->refcnt++;
	release(&bcache.buckets[HASH(b->blockno)].bucketLock);
}

void bunpin(struct buf *b) {
	acquire(&bcache.buckets[HASH(b->blockno)].bucketLock);
	b->refcnt--;
	release(&bcache.buckets[HASH(b->blockno)].bucketLock);
}
