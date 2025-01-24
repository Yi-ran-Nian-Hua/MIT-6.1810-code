//
// Created by 尹彦江 on 25-1-23.
//

#include "user/user.h"

/**
 * 用于挑选出要筛选的数字
 * @param arr 要处理的数组
 */
__attribute__((noreturn)) // 为了避免编译器报错无限递归, 添加这一行指令
void grepNumber(int pipefd[2]) {
	// 递归出口
	int number;
	read(pipefd[0], &number, sizeof(int));
	if (number == -1) {
		exit(0);
	}
	printf("prime %d\n", number);

	// 创建当前进程的管道
	int pipeRight[2];
	if (pipe(pipeRight) == -1) {
		fprintf(2, "pipe error\n");
		exit(1);
	}

	if (fork() == 0) {
		// 子进程读取数据, 继续递归调用
		close(pipefd[0]);
		grepNumber(pipeRight); // 递归调用

	}else {
		// 父进程需要筛选出数字并且将其传给子进程
		// 关闭父进程右侧读窗口
		close(pipeRight[0]);
		int result;
		while (read(pipefd[0], &result, sizeof(result)) && result != -1) {
			if (result % number != 0) {
				write(pipeRight[1], &result, sizeof(int));
			}
		}
		result = -1;
		write(pipeRight[1], &result, sizeof(int));
		close(pipeRight[1]);
		wait(0);
		exit(0);
	}
}

/**
 * primers函数的实现
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[]) {

	int pipefd[2]; // 管道, pipe[0] 表示读通道, pipe[1] 表示写通道
	if (pipe(pipefd) == -1) {
		fprintf(2, "pipe error!\n");
		exit(1);
	}

	if (fork() == 0) {
		// 子进程逻辑
		// 读取父进程数据
		// fork, 并将这些数据发送给父进程, 之后父进程需要进行筛选
		// 发送完毕后关闭管道
		close(pipefd[1]);
		grepNumber(pipefd);
		close(pipefd[0]);
		exit(0);
	}else {
		close(pipefd[0]);
		// 父进程逻辑
		// 将数据依次丢给子进程
		for (int i  = 2; i <= 34; ++i) {
			write(pipefd[1], &i, sizeof(int)); // 将数据写入子进程中
		}
		int end = -1; // 结束标志
		write(pipefd[1], &end, sizeof(int)); // 将结束标志写入子进程中
		close(pipefd[1]);
	}
	wait(0);
	exit(0);
}