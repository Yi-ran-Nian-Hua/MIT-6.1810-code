/**
 * ls 功能的具体实现
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

/**
 *
 * @param path
 * @return
 */
char* fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  // 首先, p 指向了 path 的最后一个字符, 向前寻找, 直到找到'/'停止
  // 如果没有找到'/', 遍历到头也停止
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  // 这里 p 会指向'/'后的第一个字符的位置
  p++;

  // Return blank-padded name.
  // 如果这一小段已经超出了 DIRSIZ 就直接返回
  if(strlen(p) >= DIRSIZ)
    return p;
  // 如果没有超出, 就将这部分的内容复制到 buf 中, 并将剩下部分的内容填充上空格
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de; // 存储目录信息
  struct stat st; // 存储文件信息

  // 首先打开 path 的文件
  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }
  // 之后获取该文件的文件信息以便于和后续判断文件类型
  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // 根据文件类型进行处理
  switch(st.type){
  case T_DEVICE:
  case T_FILE:
    // 如果已经是文件类型或者设备文件类型, 则直接输出信息即可
    printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;
  // 如果是目录, 则仍需要进一步获取信息
  case T_DIR:
    // 首先判断路径名称是否已经超过长度, 如果超过长度则直接返回
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf); // p 指向 buf 的最后一个地方
    *p++ = '/'; // 在这个地方的后面一个位置添加'/'
    // 读取 de 的信息
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ); // 保存目录的名字
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[])
{
  int i;

  // 如果用户传入参数小于两个, 则执行"ls ."操作, 列出当前目录信息
  if(argc < 2){
    ls(".");
    exit(0);
  }
  // 如果比两个多或者等于两个, 就依次执行遍历目录操作
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}
