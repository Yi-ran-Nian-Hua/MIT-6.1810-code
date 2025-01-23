/**
 * 该头文件定义了 stat 结构体
 * 该结构体描述了一个 iNode 的基本信息
 */
#define T_DIR     1   // 目录
#define T_FILE    2   // 文件
#define T_DEVICE  3   // 设备

struct stat {
  int dev;     // 文件系统的磁盘设备
  uint ino;    // iNode 编号
  short type;  // 文件类型
  short nlink; // 指向该文件的连接数量
  uint64 size; // 文件大小(文件字节数)
};
