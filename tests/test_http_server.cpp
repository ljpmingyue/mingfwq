#include "../mingfwq/http/http_server.h"
#include "../mingfwq/log.h"
#include "../mingfwq/http/servlet.h"

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void run(){
    mingfwq::http::HttpServer::ptr server(new mingfwq::http::HttpServer);
    mingfwq::Address::ptr addr = mingfwq::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)){
        sleep(2);
    }
    auto ad = server->getServletDispath();
    ad->addServlet("/mingfwq/xx",[](mingfwq::http::HttpRequest::ptr req,
                                mingfwq::http::HttpResponse::ptr res,
                                mingfwq::http::HttpSession::ptr session){
        res->setBody(req->toString());
        return 0;
    });

    ad->addServlet("/mingfwq/*",[](mingfwq::http::HttpRequest::ptr req,
                                mingfwq::http::HttpResponse::ptr res,
                                mingfwq::http::HttpSession::ptr session){
        res->setBody("Glob:\r\n" + req->toString());
        return 0;
    });
    server->start();
    
}


int main(){
    mingfwq::IOManager iom(2);
    iom.schedule(run);
    return 0;
}