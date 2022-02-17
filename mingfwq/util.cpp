#include "util.h"
#include "log.h"
#include "fiber.h"

#include <execinfo.h>
#include <sys/time.h>

namespace mingfwq{

mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");

pid_t GetThreadId(){

	return syscall(SYS_gettid);
}
uint32_t GetFiberId(){
	return mingfwq::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip){
	//栈的大小一般设置比较小，而线程的栈可能会比较大，轻量级切换，一方面会产生很多协程
	//所以一般不在栈创建很大的对象
	void** array = (void**)malloc((sizeof(void*) * size));
	size_t s = ::backtrace(array,size);

	char** strings = backtrace_symbols(array, s);
	if(strings == NULL){
		MINGFWQ_LOG_ERROR(g_logger) << "backtrace_symbols error";
		return;
	}

	for(size_t i = skip; i < s; ++i){
		bt.push_back(strings[i]);
	}
	
	free(strings);
	free(array);
}
std::string BacktraceToString(int size, int skip, const std::string prefix){
	std::vector<std::string> bt;
	Backtrace(bt , size,skip);
	std::stringstream ss;
	for(size_t i = 0; i < bt.size(); ++i){
		ss << prefix << bt[i] << std::endl;
	}
	return ss.str();
}

uint64_t GetCurrentMS(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//毫秒 = 秒*1000 + 纳秒/1000
	return tv.tv_sec * 1000ul + tv.tv_usec /1000;
}
uint64_t GetCurrentUS(){
	 struct timeval tv;
	gettimeofday(&tv, NULL);
	//毫秒 = 秒*1000 + 纳秒/1000
	return tv.tv_sec * 1000 * 1000ul + tv.tv_usec ;
}

}
