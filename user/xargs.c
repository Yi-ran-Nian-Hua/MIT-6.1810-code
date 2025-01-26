//
// Created by 尹彦江 on 25-1-26.
//


#include "user/user.h"

int main(int argc, char* argv[]) {
	/**
	 * 1. 首先获取要执行的指令，对应argv[1]
	 * 2. 之后将剩下的参数都收集起来
	 * 3. 处理标准输入中的结果, 把每一行的输出拼在刚才收集的参数后面, 之后分出子线程来执行
	 * 如此往复, 直到标准输入中的所有结果都处理完停止
	 */
	char command[20]; // 保存指令
	char* args; // 保存参数列表
	int offset = 0; // 记录偏移量
	char *pointer = args; // 记录args最后的位置
	char* beforeChangeLinePointer; // 记录在换行符出现前的args位置
	int isExec = 0; // 用于表示上一行指令是否已经执行完毕, 0表示没有, 1表示有
	// 1. 收集要执行的指令
	strcpy(command, argv[1]);
	// 2. 保存剩下的所有参数
	// 2.1 首先保存argv数组中剩下的所有参数
	for (int i = 2; i < argc; ++i) {
		memmove(args + offset, argv[i], strlen(argv[i]));
		offset += strlen(argv[i]) + 1; // 记录当前位置
		args[offset] = ' '; // 复制完指令之后, 将后面一个字符变成空格
		offset++;
		pointer = args + offset;
	}
	// 2.2 之后保存标准输入中的所有结果

	while (read(0, pointer, 1) != 0) {
		if (*pointer == '\n') {
			// 如果读到换行, 就代表这一行数据已经读取完毕, 此时直接运行即可
			beforeChangeLinePointer = pointer - 1;
			*pointer = 0; // 将其变成字符串
			if (fork() == 0) {
				exec(command, args);
				exit(0);
			} else {
				wait(0);
				isExec = 1;
			}
		}else {
			// 首先需要判断是否是上一个程序执行后的继续读取
			if (isExec == 1) {
				memmove(beforeChangeLinePointer, pointer, 1);
				isExec = 0;
				// 如果是上一次执行结束, 则需要回到原始位置重新进行读取
			}else {
				// 否则继续读取即可
				memmove(args + offset, pointer, 1);
				offset ++; // 记录当前位置
				pointer++;
			}
		}
	}
	exit(0);
}