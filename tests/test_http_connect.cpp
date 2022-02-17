#include <iostream>
#include "../mingfwq/http/http_connection.h"
#include "../mingfwq/log.h"
#include "../mingfwq/http/http_parser.h"
#include "../mingfwq/http/http_server.h"
#include <fstream>

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void test_pool(){
    mingfwq::http::HttpConnectionPool::ptr pool(new mingfwq::http::HttpConnectionPool(
                            "www.sylar.top","", 80, 10, 1000 * 30, 5));
    
    mingfwq::IOManager::GetThis()->addTimer(1000,[pool](){
        auto rt = pool->doGet("/",300);
        MINGFWQ_LOG_INFO(g_logger) << rt->toString();
    },true);
}


void run(){
    /*
    mingfwq::Address::ptr addr = mingfwq::Address::LookupAnyIPAddress("www.sylar.top:80");
    if(!addr){
        MINGFWQ_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    mingfwq::Socket::ptr sock = mingfwq::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt){
        MINGFWQ_LOG_INFO(g_logger) << "connect " << addr << "failed";
        return;
    }

    mingfwq::http::HttpConnection::ptr conn(new mingfwq::http::HttpConnection(sock));
    mingfwq::http::HttpRequest::ptr req(new mingfwq::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
    

    MINGFWQ_LOG_INFO(g_logger) << "req:" << std::endl
                    << *req;
    
    conn->sendRequest(req);
    auto tmp = conn->recvResponse();
    if(!tmp){
        MINGFWQ_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    MINGFWQ_LOG_INFO(g_logger) << "tmp:" << std::endl
                    << *tmp;

    std::ofstream ofs("res.dat");
    ofs << *tmp;
*/
    MINGFWQ_LOG_INFO(g_logger) << "--------------------------------";

    mingfwq::http::HttpResult::ptr r = mingfwq::http::HttpConnection::DoGet("http://www.sylar.top/", 300);
    MINGFWQ_LOG_INFO(g_logger) << "result = " << r->result 
                                << ",error" << r->error
                                << ",res = " << (r->response? r->response->toString() : "");

    MINGFWQ_LOG_INFO(g_logger) << "--------------------------------";
    test_pool();
}

int main(){
    mingfwq::IOManager iom(2);
    iom.schedule(run);
    return 0;
}