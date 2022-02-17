#pragma once

#include "address.h"
#include "noncopyable.h"
#include <memory>



namespace mingfwq{

class Socket: public std::enable_shared_from_this<Socket> , Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    //socket类型
    enum Type{
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM 
    };
    
    //socket协议簇
    enum Family{
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        Unix = AF_UNIX
    };

    static Socket::ptr CreateTCP(mingfwq::Address::ptr address);
    static Socket::ptr CreateUDP(mingfwq::Address::ptr address);

    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();

    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();

    static Socket::ptr CreateUnixTCPSocket();
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family, int type, int protocol = 0);
    ~Socket();
    //获取发送超时时间 ms,从fd_manager中拿出来
    int64_t getSendTinmeout();
    //设置发送超时时间， 通过setsockopt设置到套接字选项中
    void setSendTimeout(int64_t v);

    //获取,设置接受超时 时间 ms
    int64_t getRecvTinmeout();
    void setRecvTimeout(int64_t v);

    //获取sockopt, getsockopt 获取套接字信息
    bool getOption(int level, int option, void* result, socklen_t* len);
    template<class T>
    bool getOption(int level, int option, T& result){
        socklen_t length = sizeof(result);
        return getOption(level, option, &result, &length);
    }

    //设置sockopt, setsockopt 获取套接字信息
    bool setOption(int level, int option, void* result, socklen_t len);
    template<class T>
    bool setOption(int level, int option, T& result){
        socklen_t length = sizeof(result);
        return setOption(level, option, &result, length);
    }

    //接受connnect连接
    Socket::ptr accept();
    //绑定地址
    bool bind(const Address::ptr addr);
    //连接地址
    bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    //监听socket
    bool listen(int backlog = SOMAXCONN);
    //关闭socket
    bool close();

    int send(const void* buffer, size_t length, int flags = 0);
    int send(const iovec* buffers, size_t length, int flags = 0);
    int sendTo(const void* buffer, size_t length, const Address::ptr to,int flags = 0);
    int sendTo(const iovec* buffers, size_t length, const Address::ptr to,int flags = 0);

    int recv(void* buffer, size_t length, int flags = 0);
    int recv(iovec* buffers, size_t length, int flags = 0);
    int recvFrom(void* buffer, size_t length, const Address::ptr from, int flags = 0);
    int recvFrom(iovec* buffers, size_t length, const Address::ptr from, int flags = 0);

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();

    int getFamily()const{return m_family;}
    int getType()const{return m_type;}
    int getProtocol()const{return m_protocol;}

    bool isConnected(){return m_isConnected;}
    //是否有效（m_sock != -1）
    bool isValid();
    int getError();
 
    //输出信息到流中
    std::ostream& dump(std::ostream& os)const;

    int getSocket()const{return m_sock;}

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll(); 
private:
    //初始化socket
    void initSock();
    //创建socket
    void newSock();
    //初始化sock
    bool init(int sock);
private:
    //socket句柄
    int m_sock;
    //协议簇
    int m_family;
    //类型
    int m_type;
    //协议
    int m_protocol;
    //是否连接
    bool m_isConnected;
    //本地地址
    Address::ptr m_localAddress;
    //远端地址
    Address::ptr m_remoteAddress;

};

    /*
    **  流式输出socket
    **  os 输出流, sock Socket类
    */
    std::ostream& operator<<(std::ostream& os, const Socket& addr);



}