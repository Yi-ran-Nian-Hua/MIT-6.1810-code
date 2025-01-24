// xv-6 支持的系统调用
#include "kernel/types.h"
struct stat; // iNode 结构体

// system calls

/**
 * 创建一个进程
 * @return 子进程的 pid
 */
int fork(void);

/**
 * 终止当前进程
 * @param status 以何种状态返回, 并将状态报告给 wait 函数
 * @return 无返回
 */
int exit(int status) __attribute__((noreturn));

/**
 * 等待一个子进程退出
 * @param status 退出的状态
 * @return 子进程 pid
 */
int wait(int * status);

/**
 * 创建一个管道，把read/write文件描述符放在p[0]和p[1]中
 * @param p read 文件描述符对应 p[0], write 文件描述符对应 p[1]
 * @return 
 */
int pipe(int *p);

/**
 * 从 buffer 写 n 个字节到文件描述符 fd; 返回 n
 * @param fd 文件描述符
 * @param buffer 要写入的数据
 * @param n 要写入的字节个数
 * @return 
 */
int write(int fd, const void * buffer, int n);

/**
 * 将 n 个字节读入 buf；
 * @param fd  文件描述符
 * @param buffer 读取的文件存储的位置
 * @param n 读取字节个数
 * @return 读取字节数, 如果文件结束返回 0
 */
int read(int fd, void * buffer, int n);

/**
 * 释放打开的文件 fd
 * @param fd 要关闭的文件描述符
 * @return 
 */
int close(int fd);

/**
 * 终止对应 pid 的进程
 * @param pid 要关闭的 pid
 * @return 0 表示成功关闭, -1 表示错误
 */
int kill(int pid);

/**
 * 加载一个文件并使用参数执行它
 * @param file 要加载的文件路径
 * @param argv 执行时携带的参数列表
 * @return 只有在出错的时候才会返回
 */
int exec(const char * file, char ** argv);

/**
 * 打开一个文件
 * @param file 文件路径
 * @param flags 如何对文件进行操作?读还是写? 
 * @return 文件描述符 fd
 */
int open(const char * file, int flags);

/**
 * 创建一个设备文件
 * @param file
 * @return 
 */
int mknod(const char * file, short, short);

/**
 * 删除一个文件
 * @param file 文件路径
 * @return
 */
int unlink(const char * file);

/**
 * 将打开文件 fd 的信息存放到结构体中
 * @param fd 打开的文件描述符
 * @param st 信息存储位置
 * @return
 */
int fstat(int fd, struct stat * st);

/**
 * 为文件 file1 创建另外一个名称 file2
 * @param file1 文件 1
 * @param file2 文件 2
 * @return
 */
int link(const char * file1, const char * file2);

/**
 * 创建一个新目录dir
 * @param dir 目录名称
 * @return 
 */
int mkdir(const char * dir);

/**
 * 改变当前的工作目录
 * @param dir 要改变到的目录名
 * @return 
 */
int chdir(const char * dir);

/**
 * 返回一个新的文件描述符, 指向与 fd 相同的文件
 * @param fd 要指向的文件描述符
 * @return 新的文件描述符, 指向与 fd 相同的文件
 */
int dup(int fd);

/**
 * 返回当前进程 pid
 * @return 当前进程 pid
 */
int getpid(void);

/**
 * 按 n 字节增长进程的内存, 返回新内存的开始
 * @param n 增长内存大小
 * @return 新内存的开始
 */
char *sbrk(int n);

/**
 * 暂停 n 个时钟节拍
 * @param n 时钟节拍个数
 * @return 
 */
int sleep(int n);

/**
 * 
 * @return
 */
int uptime(void);

// ulib.c
int stat(const char *, struct stat *);
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);

/**
 * 返回 src 中第一个出现 c 的位置
 * @param src 要操作的字符串
 * @param c 要寻找的字符
 * @return
 */
char *strchr(const char * src, char c);
int strcmp(const char *, const char *);
void fprintf(int, const char *, ...);
void printf(const char *, ...);
char *gets(char *, int max);
uint strlen(const char *);
void *memset(void *, int, uint);
void *malloc(uint);
void free(void *);
int atoi(const char *);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
