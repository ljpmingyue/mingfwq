#include "tcp_server.h"
#include "config.h"
#include "log.h"


namespace mingfwq{

    static mingfwq::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = 
            mingfwq::Config::Lookup("tcp_server.readtimeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");
    static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");

    TcpServer::TcpServer(mingfwq::IOManager* worker,mingfwq::IOManager* accept_worker)
            :m_worker(worker)
            ,m_acceptworker(accept_worker)
            ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
            ,m_name("mingfwq/1.0.0")
            ,m_isStop(true){

    }
    TcpServer::~TcpServer(){
        for(auto& i : m_socks){
            i->close();
        }
        m_socks.clear();
    }

    bool TcpServer::bind(mingfwq::Address::ptr addr){
        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> fails;
        addrs.push_back(addr);
        return bind(addrs,fails);
    }

    bool TcpServer::bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails){
        for(auto& addr : addrs){
            Socket::ptr sock = mingfwq::Socket::CreateTCP(addr);
            if(!sock->bind(addr)){
                MINGFWQ_LOG_ERROR(g_logger) << "bind fail errno: " << errno
                        << " errstr = " << strerror(errno)
                        << " addr =[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            if(!sock->listen()){
                MINGFWQ_LOG_ERROR(g_logger) << "listem fail errno: " << errno
                        << " errstr = " << strerror(errno)
                        << " addr =[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);
        }

        if(!fails.empty()){
                m_socks.clear();
                return false;
        }

        for(auto& i : m_socks){
            MINGFWQ_LOG_INFO(g_logger) << "server bind success: " << *i << std::endl;
        }
        return true;
    }

    void TcpServer::startAccpet(Socket::ptr sock){
        while (!m_isStop){
            Socket::ptr client = sock->accept();
            if(client){
                client->setRecvTimeout(m_recvTimeout);
                m_worker->schedule(std::bind(&TcpServer::handleClient,
                                shared_from_this(), client));
            }else{
                MINGFWQ_LOG_ERROR(g_logger) << "accept errno = " << errno
                                << " errstr = " << strerror(errno);
            }
        }
        
    }

    bool TcpServer::start(){
        if(!m_isStop){
            return true;
        }
        m_isStop = false;

        for(auto& sock : m_socks){
            m_acceptworker->schedule(std::bind(&TcpServer::startAccpet, 
                            shared_from_this(), sock));
        }
        return true;
    }

    void TcpServer::stop(){
        m_isStop = true;
        auto self = shared_from_this();
        m_acceptworker->schedule([this, self](){
            for(auto& sock: self->m_socks){
                sock->cancelAll();
                sock->close();
            }
            self->m_socks.clear();
            
        });
    }

    void TcpServer::handleClient(Socket::ptr client){
        MINGFWQ_LOG_INFO(g_logger) << "handleClient: " << *client;
    }
}