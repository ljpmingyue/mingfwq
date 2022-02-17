#pragma once
#include <memory>
#include <string>

#include "http11_parser.h"
#include "httpclient_parser.h"
#include "http.h"


namespace mingfwq{
namespace http{

//HTTP请求解析类
class HttpRequestParser{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();
    //解析
    //data 数据 ， len 总长度， off 偏移量
    size_t execute(char* data, size_t len, size_t off = 0);
    //是否解析结束
    int isFinished();
    //返回是否有错
    int hasError();
    //返回HttpRequest结构体
    HttpRequest::ptr getData()const{return m_data;}
    //设置错误
    void setError(int v){m_error = v;}
    //获取消息体长度
    uint64_t getContentLength();
    const http_parser& getParser()const{return m_parser;}
public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    /*错误码：
    **  1000: invalid method
    **  1001: invalid version
    **  1002: invalid field
    */
    int m_error;
};

//Http响应解析结构体
class HttpResponseParser{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    size_t execute(char* data, size_t len, bool chunck = false ,size_t off = 0);
    int isFinished();
    int hasError();

    HttpResponse::ptr getData()const {return m_data;}
    void setError(int v){m_error = v;}
    //获取消息体长度
    uint64_t getContentLength();
    const httpclient_parser& getParser()const{return m_parser;}
public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    /*错误码：
    **  1001: invalid version
    **  1002: invalid field
    */ 
    int m_error;

};


}


}