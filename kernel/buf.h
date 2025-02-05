/**
 * 一个 buffer 的内容
 */
struct buf {
  int valid;   // has data been read from disk? cache 中是否包含块的副本
  int disk;    // does disk "own" buf? cache 中的内容是否已经交给磁盘
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

