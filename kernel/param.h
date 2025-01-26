#define NPROC        64  // maximum number of processes, 进程数的最大数量
#define NCPU          8  // maximum number of CPUs, CPU的最大数量
#define NOFILE       16  // open files per process, 单独进程允许打开的最大文件数量
#define NFILE       100  // open files per system,  每个系统允许打开的最大文件数量
#define NINODE       50  // maximum number of active i-nodes, 允许活跃的iNode最大个数
#define NDEV         10  // maximum major device number,
#define ROOTDEV       1  // device number of file system root disk, 文件系统根磁盘设备号
#define MAXARG       32  // max exec arguments, 每个程序允许执行的最多参数
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes, 文件系统写入操作最大块的个数
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log,
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name, 文件名路径最大长度
