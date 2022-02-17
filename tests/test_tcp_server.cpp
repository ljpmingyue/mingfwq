#include "../mingfwq/log.h"
#include "../mingfwq/tcp_server.h"
#include "../mingfwq/iomanager.h"

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void run(){
    auto addr = mingfwq::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = mingfwq::UnixAddress::ptr(new mingfwq::UnixAddress("/tmp/unix_addr"));

    std::vector<mingfwq::Address::ptr> addrs;
    std::vector<mingfwq::Address::ptr> fails;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    mingfwq::TcpServer::ptr tcp_server(new mingfwq::TcpServer);
    while(!tcp_server->bind(addrs,fails)){
        sleep(2);
    }
    MINGFWQ_LOG_INFO(g_logger) << "tcp_server->start():";
    tcp_server->start();

    //MINGFWQ_LOG_INFO(g_logger) << *addr << " - " << *addr2;
}



int main(){
    mingfwq::IOManager iom(2);
    iom.schedule(run);


    return 0;
}