#include "../mingfwq/mingfwq.h"


static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();


void run_in_fiber(){
    MINGFWQ_LOG_INFO(g_logger) << "run_in_fiber begin";
    mingfwq::Fiber::YieldToHold();
    MINGFWQ_LOG_INFO(g_logger) << "run_in_fiber end";
    mingfwq::Fiber::YieldToHold();
}

void test_fiber(){
    
    MINGFWQ_LOG_INFO(g_logger) << "main begin -1 ";
    {
        mingfwq::Fiber::GetThis();
        MINGFWQ_LOG_INFO(g_logger) << "main begin";
        mingfwq::Fiber::ptr fiber(new mingfwq::Fiber(run_in_fiber));
        fiber->swapIn();
        MINGFWQ_LOG_INFO(g_logger) << "main after swapin";
        fiber->swapIn();
        MINGFWQ_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    MINGFWQ_LOG_INFO(g_logger) << "main after end -2";
}
int main(){
    mingfwq::Thread::SetName("main");

    std::vector<mingfwq::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i){
        thrs.push_back(mingfwq::Thread::ptr(new mingfwq::Thread(&test_fiber,"name_" + std::to_string(i))));
    }
    for(auto i : thrs){
        i->join();
    }

    return 0;
}