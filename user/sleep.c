//
// Created by 尹彦江 on 25-1-23.
//
#include "user/user.h"

int main(int argc, char const *argv[])
{
    int sleepTime = 0; // 睡眠时间

    if (argc < 2)
    {
        fprintf(2, "Usage: sleep sleep_times\n");
        exit(1);
    }
    sleepTime = atoi(argv[1]); // 将睡眠时间转换为 int
    sleep(sleepTime);

    exit(0); // 退出程序

}
