//
// Created by 尹彦江 on 25-1-24.
//

#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

void find(char* path, char* target) {
	char buffer[512], *p;
	int fileDescription; // 用于存储文件描述符
	struct dirent dirInfo; // 用于保存目录信息
	struct stat fileInfo; // 用于保存文件信息

	// 首先打开路径下的文件
	if ((fileDescription = open(path, 0)) < 0) {
		fprintf(2, "find: cannot open: %s\n", path);
		return;
	}
	// 之后获取目录下的信息
	if(fstat(fileDescription, &fileInfo) < 0){
		fprintf(2, "find: cannot stat %s\n", path);
		close(fileDescription);
		return;
	}

	// 根据文件类型进行处理
	switch(fileInfo.type){
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

int main(int argc, char* argv[]) {
	// find 用法: find [搜索路径] [搜索文件]
	if (argc < 3) {
		fprintf(2, "Usage: find path file_name\n");
		exit(1);
	}

	// 将命令存储下来
	char path[1024];
	char target[1024];
	strcpy(path, argv[1]);
	strcpy(target, argv[2]);
	find(path, target);
	exit(0);
}