#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
#define O_TRUNC   0x400


//mmap prot参数选项
#ifdef LAB_MMAP
#define PROT_NONE       0x0 // 内存映射为不可读/不可写
#define PROT_READ       0x1 // 内存映射为可读
#define PROT_WRITE      0x2 // 内存映射为可写
#define PROT_EXEC       0x4 // 内存映射为可执行

// mmap flags 参数选项
#define MAP_SHARED      0x01 // 映射内存的修改应写回文件
#define MAP_PRIVATE     0x02 // 映射内存的修改不应写回文件
#endif
