#pragma once 

#include <memory>
#include <functional>

#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "noncopyable.h"

namespace mingfwq{

class TcpServer:public std::enable_shared_from_this<TcpServer>
                ,Noncopyable{
public:
    typedef std::shared_ptr<TcpServer> ptr;
    /* worker socket客户端工作的协程调度器
    **  accept_worker 服务器socket执行接收socket连接的协程调度器
    */
    TcpServer(mingfwq::IOManager* worker = mingfwq::IOManager::GetThis()
                ,mingfwq::IOManager* accept_worker = mingfwq::IOManager::GetThis());

    virtual ~TcpServer();
    //绑定地址 0成功 -1失败
    virtual bool bind(mingfwq::Address::ptr addr);
    //绑定地址数组  fails 绑定失败的地址
    virtual bool bind(const std::vector<Address::ptr>& addrs, 
                    std::vector<Address::ptr>& fails);
    //启动服务, 需要bind成功后执行
    virtual bool start();
    //停止服务
    virtual void stop();
    //返回读取超时时间
    uint64_t getRecvTimeout()const{return m_recvTimeout;}
    std::string getName()const{return m_name;}
    void setRecvTimeout(uint64_t v){m_recvTimeout = v;}
    void setName(const std::string& v){m_name = v;}

    bool isStop()const{return m_isStop;}
private:
    //处理新连接的Socket类
    virtual void handleClient(Socket::ptr client);
    //开始接受连接
    virtual void startAccpet(Socket::ptr sock); 
private:
    // 监听Socket数组
    std::vector<Socket::ptr> m_socks;
    // 新连接的Socket工作的调度器
    IOManager* m_worker;
    // 服务器Socket接收连接的调度器
    IOManager* m_acceptworker;
    // 接收超时时间(毫秒)
    uint64_t m_recvTimeout;
    // 服务器名称
    std::string m_name;
    // 服务是否停止
    bool m_isStop;
};

}