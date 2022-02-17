#pragma once

#include <memory>
#include "../socket_stream.h"
#include "http.h"



namespace mingfwq{
namespace http{

//HTTPSession封装  server.socket->session
class HttpSession: public SocketStream{
public:
    typedef std::shared_ptr<HttpSession> ptr;
    
    HttpSession(Socket::ptr sock, bool owner = true);

    //接收HTTP请求
    HttpRequest::ptr recvRequest();

    /*  发送HTTP响应，hrt HTTP响应
    **  >0 发送成功，=0 对方关闭，<0 Socket异常
    */
    int sendResponse(HttpResponse::ptr hrt);
};



}

}


