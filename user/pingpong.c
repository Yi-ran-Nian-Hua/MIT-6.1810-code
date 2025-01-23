//
// Created by 尹彦江 on 25-1-23.
//

#include "user/user.h"

/**
 * pingpong 函数的实现
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv) {
	int pipefd[2]; // 定义管道, 其中pipe[0]表示读通道, pipe[1]表示写通道
	if (pipe(pipefd) == -1) {
		// 如果创建管道失败提示错误信息
		fprintf(2, "pipe error\n");
		exit(1);
	}

	char buffer[1024]; // 数据缓冲区

	if (fork() == 0) {
		// 子进程逻辑
		read(pipefd[0], buffer, sizeof(buffer)); // 读取读通道中的数据
		fprintf(1, "%d: received ping\n", getpid());
		write(pipefd[1], "c", 2);
		exit(0);

	}else {
		// 父进程逻辑
		write(pipefd[1], "f", 2);
		read(pipefd[0], buffer, sizeof(buffer));
		fprintf(1, "%d: received pong\n", getpid());
		wait(0); // 等待子进程结束
	}

	exit(0);
}