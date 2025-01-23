/**
 * echo 程序实现
 */
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
 *
 * @param argc 用户传入的参数个数,argc = 用户传入参数 + 1, 因为 argv[0]表示的是程序本身
 * @param argv 用户传入的参数内容
 * @return
 */
int main(int argc, char *argv[])
{
  int i; // 循环变量

  /* 从 argv[1] 开始依次读取内容 */
  for(i = 1; i < argc; i++){
    write(1, argv[i], strlen(argv[i])); // 将 argv[i] 的内容写入到标准输出中
    if(i + 1 < argc){
      /* 每一个参数之间使用空格隔开 */
      write(1, " ", 1);
    } else {
      /* 最后输出换行 */
      write(1, "\n", 1);
    }
  }
  exit(0); // 结束程序
}
