/**
 * 该头文件表示了 open()函数的标志位, 表示打开的操作
 */
#define O_RDONLY  0x000 // 只读
#define O_WRONLY  0x001 // 只写
#define O_RDWR    0x002 // 可读可写
#define O_CREATE  0x200 // 如果文件不存在则创建文件
#define O_TRUNC   0x400 // 将文件截断为零长度
