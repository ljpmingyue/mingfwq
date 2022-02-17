#include "http_server.h"
#include "../log.h"

namespace mingfwq{
namespace http{

    static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");

    HttpServer::HttpServer(bool Keepalive
            ,mingfwq::IOManager* worker ,mingfwq::IOManager* accept_work)
            :TcpServer(worker,accept_work)
            ,m_isKeepalive(Keepalive)
            ,m_dispath(new ServletDispath){
  
        }


    void HttpServer::handleClient(Socket::ptr client){
        mingfwq::http::HttpSession::ptr session(new HttpSession(client));
        do{
            auto req = session->recvRequest();
            if(!req){
                MINGFWQ_LOG_ERROR(g_logger) << "recv http request fail, errno = " 
                        << errno << ",strerr = " << strerror(errno)
                        << ",client = " << *client << ",keep_alive = " << m_isKeepalive;
                break;
            }
            
            HttpResponse::ptr res(new HttpResponse(req->getVersion(), req->isClose()|| !m_isKeepalive));
            res->setHeader("Server", getName());

            m_dispath->handle(req,res,session);

            //res->setBody("hello mingfwq HttpServer::handleClient");             
            //MINGFWQ_LOG_INFO(g_logger) << "request:" << std::endl << *req;
            //MINGFWQ_LOG_INFO(g_logger) << "response:" <<std::endl << *res;
            
            //为什么session不添加到Servlet里面去处理，因为servlet可能需要多层处理，每一次可能会添加东西
            session->sendResponse(res);

        }while (m_isKeepalive);
        session->close();
    }

}
}