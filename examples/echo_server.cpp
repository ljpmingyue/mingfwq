#include "../mingfwq/tcp_server.h"
#include "../mingfwq/log.h"
#include "../mingfwq/iomanager.h"
#include "../mingfwq/bytearray.h"
#include "../mingfwq/address.h"
#include "../mingfwq/address.h"

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

class EchoServer:public mingfwq::TcpServer{
public:
    typedef std::shared_ptr<EchoServer> ptr;

    EchoServer(int type);

    void handleClient(mingfwq::Socket::ptr client);

private:
    // 1: text
    // 其他：二进制
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    :m_type(type){

}
    
void EchoServer::handleClient(mingfwq::Socket::ptr client){
    MINGFWQ_LOG_INFO(g_logger) << "handleClinet " << *client;
    mingfwq::ByteArray::ptr ba(new mingfwq::ByteArray);

    while (true){
        ba->clear();
        std::vector<iovec> ioves;
        ba->getWriteBuffers(ioves,1024);

        int rt = client->recv(&ioves[0],ioves.size());
        if(rt == 0){
            MINGFWQ_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        }else if(rt < 0){
            MINGFWQ_LOG_INFO(g_logger) << "client error rt = " << rt 
                        << " errno = " << errno << " strerr = " << strerror(errno);
            break;
        }
        
        //ba->setPosition(ba->getPosition()+rt);//不明白为什么要再设置一次
        //更新m_size ?
        ba->setPosition(rt + ba->getPosition());
        ba->setPosition(0);
        //MINGFWQ_LOG_INFO(g_logger) << "recv rt = " << rt << " rt = " << std::string((char*)ioves[0].iov_base, rt);

        if(m_type == 1){//text
            std::cout << ba->toString();
        }else{
            std::cout << ba->toHexString();
        }
        std::cout.flush();
    }
    
}
int type = 1;

void run(){
    EchoServer::ptr es(new EchoServer(type));
    auto addr = mingfwq::Address::LookupAny("0.0.0.0:8020");
    while (!es->bind(addr)){
        sleep(2);
    }
    es->start();
}

int main(int argc,char** argv){
    if(argc < 2){
        MINGFWQ_LOG_INFO(g_logger) << "used as[" << argv[0] << "-t] or [" << argv[0] <<"-b]";
        return 0;
    }
    if(!strcmp(argv[1], "-b")){
        type = 2;
    }

    mingfwq::IOManager iom(2);
    iom.schedule(run);

    return 0;

}