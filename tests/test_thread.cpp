#include "../mingfwq/mingfwq.h"
#include <unistd.h>

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

float count = 0;
//mingfwq::RWMutex s_mutex; 
mingfwq::Mutex s_mutex;

void fun1(){
    MINGFWQ_LOG_INFO(g_logger)  << "name:" << mingfwq::Thread::GetName()
                                << " this.name: " << mingfwq::Thread::GetThis()->getName()
                                << " id: " << mingfwq::GetThreadId()
                                << " this.id:" <<mingfwq::Thread::GetThis()->getId();

    for(int i = 0; i < 50000; ++i){
        //写锁会稳定 ，读锁会比不写好点但是比写锁差，
        //mingfwq::RWMutex::ReadLock lock(s_mutex);
        //mingfwq::RWMutex::WriteLock lock(s_mutex);
        
        //mutex锁
        mingfwq::Mutex::Lock lock(s_mutex);
        count =count + 0.1;
    }
}

void fun2(){
    while(true){
        MINGFWQ_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
    
}
void  fun3(){
    while(true){
        MINGFWQ_LOG_INFO(g_logger) << "=============================";
    }

}

int main(){
    MINGFWQ_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/mingyue/c++/mingfwq/bin/conf/log2.yml");
    mingfwq::Config::LoadFromYaml(root);

    std::vector<mingfwq::Thread::ptr> thrs;
    for(size_t i = 0; i < 2; ++i){
        mingfwq::Thread::ptr thr(new mingfwq::Thread(&fun2,"name_" + std::to_string(i * 2 ) ));
        mingfwq::Thread::ptr thr2(new mingfwq::Thread(&fun3,"name_" + std::to_string(i * 2 + 1) ));
        thrs.push_back(thr);
        thrs.push_back(thr2);
        std::cout << "------------" << i << std::endl;
    }

    //sleep(20);
    for(size_t j = 0; j < thrs.size(); ++j){
        thrs[j]->join();
    }
    MINGFWQ_LOG_INFO(g_logger) << "thread test end";
    MINGFWQ_LOG_INFO(g_logger) << "count = " << count;

    

    return 0;
}