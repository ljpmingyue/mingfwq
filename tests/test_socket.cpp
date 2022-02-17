#include "../mingfwq/socket.h"
#include "../mingfwq/mingfwq.h"

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void test_socket(){
    MINGFWQ_LOG_INFO(g_logger) << "lookupanyipaddress begin";
    mingfwq::IPAddress::ptr addr = mingfwq::Address::LookupAnyIPAddress("www.baidu.com");
    MINGFWQ_LOG_INFO(g_logger) << "lookupanyipaddress end";
    if(addr){
        MINGFWQ_LOG_INFO(g_logger) << "get address:" << addr->toString();
    }else{
        MINGFWQ_LOG_INFO(g_logger) << "get address fail";
        return;
    }

    mingfwq::Socket::ptr sock = mingfwq::Socket::CreateTCP(addr);
    addr->setPort(80);
    MINGFWQ_LOG_INFO(g_logger) << "addr:" << addr->toString();
    if(!sock->connect(addr)){
        MINGFWQ_LOG_ERROR(g_logger) << "connect:" << addr->toString() << "fail";
        return;
    }else{
        MINGFWQ_LOG_ERROR(g_logger) << "connect:" << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff,sizeof(buff));
    if(rt <= 0){
        MINGFWQ_LOG_ERROR(g_logger) << "send fail rt = " << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0],buffs.size());
    if(rt <= 0){
        MINGFWQ_LOG_ERROR(g_logger) << "send fail rt = " << rt;
        return;
    }

    buffs.resize(rt);
    MINGFWQ_LOG_INFO(g_logger) << buffs;



}

int main(){
    mingfwq::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}