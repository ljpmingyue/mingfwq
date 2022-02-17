#include "../mingfwq/log.h"
#include "../mingfwq/util.h"


static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void test_logban(){
    MINGFWQ_LOG_LIMIT(g_logger, mingfwq::LogLevel::FATAL,[](){return true;}) << "xxxx";


}



int main(int argc,char* argv[]){
    
    //test_logban();
    mingfwq::Logger::ptr logger(new mingfwq::Logger);
    logger->addAppender(mingfwq::LogAppender::ptr(new mingfwq::StdoutLogAppender));
    
    mingfwq::FileLogAppender::ptr file_appender(new mingfwq::FileLogAppender("./log.txt"));
    //自定义输出格式
    mingfwq::LogFormatter::ptr fmt(new mingfwq::LogFormatter("%d%T%p%T%m%n")); //time tab message /t
    file_appender->setFormatter(fmt);
    file_appender->setLevel(mingfwq::LogLevel::ERROR);

    logger->addAppender(file_appender);

    //mingfwq::LogEvent::ptr event(new mingfwq::LogEvent(__FILE__,__LINE__,0,mingfwq::GetThreadId(),mingfwq::GetFiberId(),time(0)));
    //event->getSS() <<"event getss";
    //logger->log(mingfwq::LogLevel::DEBUG,event);
    std::cout<<"hello mingfwq log"<<std::endl;


    MINGFWQ_LOG_INFO(logger) << "test macro ";

    MINGFWQ_LOG_ERROR(logger) <<"test macro error";


    MINGFWQ_LOG_FMT_ERROR(logger,"TEST MACRO FMT ERROR %s","aa");

    auto l = mingfwq::LoggerMgr::GetInstance()->getLogger("xx");
    MINGFWQ_LOG_INFO(l) <<"XXX";
   
    return 0;
}

	//m:消息
        //p:日志级别
        //r:累计毫秒数
        //c:日志名称
        //t:线程id
        //n:换行
        //d:时间
        //f:文件名
        //l:行号

