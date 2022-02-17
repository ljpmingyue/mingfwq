#pragma once
#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"
#include <memory>

namespace mingfwq{
namespace http{
//HTTP服务器封装
class HttpServer:public TcpServer{
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool Keepalive = false
            , mingfwq::IOManager* worker = mingfwq::IOManager::GetThis()
            , mingfwq::IOManager* accept_work = mingfwq::IOManager::GetThis());

    ServletDispath::ptr getServletDispath()const{return m_dispath;}
    void setServletDispath(ServletDispath::ptr v){m_dispath = v;}
protected:
    //处理新连接的Socket类
    virtual void handleClient(Socket::ptr client);

private:
    //是否支持长连接,默认否
    bool m_isKeepalive;
    // Servlet分发器
    ServletDispath::ptr m_dispath;


};





}
}