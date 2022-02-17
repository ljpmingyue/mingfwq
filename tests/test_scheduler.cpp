#include <mingfwq/mingfwq.h>

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void test_fiber(){

    static int s_count = 5;
    MINGFWQ_LOG_INFO(g_logger) << "test in fiber s_Count = "<< s_count;
    sleep(1);
    if (--s_count >=0)
    {
        mingfwq::Scheduler::GetThis()->schedule(&test_fiber,mingfwq::GetThreadId());
    }
    
    
}


int main(){
    MINGFWQ_LOG_INFO(g_logger) << "main";
    //mingfwq::Scheduler sc(3, true, "test");
    mingfwq::Scheduler sc(2, false, "test");
    sc.start();
    MINGFWQ_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);

    sc.stop(); 
    MINGFWQ_LOG_INFO(g_logger) << "over";

    return 0;
}
