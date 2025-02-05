struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  uint timeStamp; // 时间戳, 用于后面的 LRU 算法
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

