#pragma once 

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
 
namespace mingfwq{

//获得当前线程id
pid_t GetThreadId();
//当前协程id
uint32_t GetFiberId();

/*  获得当前的调用栈
**  bt 保存调用栈，size 最多返回层数，skip 跳过栈顶的层数
*/
void Backtrace(std::vector<std::string>& bt, int size =64 , int skip = 1);
/*  获得当前的当前栈信息的字符串
**  size 栈的最大层数，skip跳过栈顶的参数，prefix 栈信息前输出的内容
*/
std::string BacktraceToString(int size = 64, int skip = 2, const std::string prefix = "");

//获取当前的时间ms,us
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();
}
