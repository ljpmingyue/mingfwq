#include "../mingfwq/http/http.h"
#include "../mingfwq/mingfwq.h"

static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

void test_request(){
    mingfwq::http::HttpRequest::ptr req(new mingfwq::http::HttpRequest());
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello mingfwq");

    req->dump(std::cout) << std::endl;

}

void test_response(){
    mingfwq::http::HttpResponse::ptr res(new mingfwq::http::HttpResponse());
    res->setHeader("X-X","mingfwq");
    res->setBody("hello mingfwq response");
    res->setStatus((mingfwq::http::HttpStatus)400);
    //res->setStatus(mingfwq::http::HttpStatus::BAD_REQUEST);
    res->setClose(false);
    res->dump(std::cout) << std::endl;
}

int main(){
    test_request();
    test_response();
    return 0;
}