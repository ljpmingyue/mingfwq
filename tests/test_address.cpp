#include "../mingfwq/mingfwq.h"


static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();


void test(){
    std::vector<mingfwq::Address::ptr> addrs;
    MINGFWQ_LOG_ERROR(g_logger) << "lookup begin";
    //bool v = mingfwq::Address::Lookup(addrs,"www.baidu.com:http");
    bool v = mingfwq::Address::Lookup(addrs,"www.sylar.top");
    MINGFWQ_LOG_ERROR(g_logger) << "lookup end";
    if(!v){
        MINGFWQ_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i){
        MINGFWQ_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
    
}

void test_iface(){
    std::multimap<std::string, std::pair<mingfwq::Address::ptr, uint32_t> > results;

    bool v = mingfwq::Address::GetInterfaceAddresses(results);
    if(!v){
        MINGFWQ_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
        return;
    }
    for(auto& i : results){
        MINGFWQ_LOG_ERROR(g_logger) <<  i.first << " - " << i.second.first->toString()
                                    << " - " << i.second.second;
        
    }

}

void test_ipv4(){
    
    auto addr = mingfwq::IPAddress::Create("www.baidu.com");
    //auto addr = mingfwq::IPAddress::Create("127.0.0.8");
    if(addr){
        MINGFWQ_LOG_INFO(g_logger) << addr->toString();
    }
    
}


int main(){

    //test_ipv4();
    test();
    //test_iface();
    

    return 0;
}   