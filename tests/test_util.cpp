#include "../mingfwq/mingfwq.h"

#include <assert.h>

mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void test_assert(){
    MINGFWQ_LOG_INFO(g_logger) << mingfwq::BacktraceToString(10);
    //MINGFWQ_ASSERT(false); 
    MINGFWQ_ASSERT2(0 == 1,"abcdef xx");
}

int main(){

    //test_assert();
    return 0;
}