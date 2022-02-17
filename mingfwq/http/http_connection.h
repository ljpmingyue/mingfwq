#pragma once
#include "../socket_stream.h"
#include "../uri.h"
#include "../thread.h"
#include "http.h"

#include <memory>
#include <list>


namespace mingfwq{
namespace http{

//HTTP响应结果
struct HttpResult{
    enum class Error{
        // 正常
        OK = 0,
        // 非法URL
        INVALID_URL = 1,
        // 无法解析HOST
        INVALID_HOST = 2,
        // 连接失败
        CONNECT_FAIL = 3,
        // 远端关闭
        SEND_CLOSE_BY_PEER = 4,
        // 发送请求产生Socket错误
        SEND_SOCKET_ERROR = 5,
        // 超时
        TIMEOUT = 6,
        // 创建socket失败
        CREATE_SOCKET_ERROR = 7,
        
        //从pool池获取失败
        POOL_GET_CONNECTION = 8,
        //无效链接 sock 无效
        POOL_INVALID_CONNECTION = 9,
    };

    typedef std::shared_ptr<HttpResult> ptr;
    
    HttpResult(int _result, HttpResponse::ptr _response, std::string _error)
            :result(_result)
            ,response(_response)
            ,error(_error){}

    std::string toString()const;
    
    int result;
    HttpResponse::ptr response;
    std::string error;
};

class HttpConnectionPool;

//HTTP客户端封装  
class HttpConnection: public SocketStream{
friend class HttpConnectionPool;
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    
    static HttpResult::ptr DoGet(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string,std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string,std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoPost(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string,std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoPost(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string,std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                , const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string,std::string>& headers = {}
                                , const std::string& body = "");
    
    static HttpResult::ptr DoRequest(HttpMethod method
                                , Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string,std::string>& headers = {}
                                , const std::string& body = "");
    
    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                                , Uri::ptr uri
                                , uint64_t timeout_ms);
    
    
    HttpConnection(Socket::ptr sock, bool owner = true);
    //接收HTTP响应
    HttpResponse::ptr recvResponse();

    /*  发送HTTP请求，hrt HTTP响应
    **  >0 发送成功，=0 对方关闭，<0 Socket异常
    */
    int sendRequest(HttpRequest::ptr req);

    ~HttpConnection();

private:
    uint64_t m_createTime = 0;
    uint64_t m_request = 0;
};

//http 客户端连接池
class HttpConnectionPool{
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;
    typedef Mutex MutexType;
public:
    HttpConnectionPool(const std::string& host
                        ,const std::string& vhost
                        ,uint32_t port
                        ,uint32_t max_size
                        ,uint32_t max_alive_time
                        ,uint32_t max_request);
    
    HttpConnection::ptr getConnection();

    HttpResult::ptr doGet(const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers = {}
                        , const std::string& body = "");

    HttpResult::ptr doGet(Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers = {}
                        , const std::string& body = "");

    HttpResult::ptr doPost(const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers = {}
                        , const std::string& body = "");

    HttpResult::ptr doPost(Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers = {}
                        , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                        , const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers = {}
                        , const std::string& body = "");
    
    HttpResult::ptr doRequest(HttpMethod method
                        , Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers = {}
                        , const std::string& body = "");

    HttpResult::ptr doRequest(HttpRequest::ptr req
                        , uint64_t timeout_ms);
private:
    //释放指针
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

private:
    std::string m_host;
    std::string m_vhost;
    int32_t m_port;
    /*  连接池最大的数量，但不是绝对的，即便不是满的也可以创建新的，
    **  不过如果使用结束之后发现池中数量大于最大量就直接释放掉
    */
    int32_t m_maxSize;
    int32_t m_maxAliveTime;
    int32_t m_maxRequest;

    MutexType m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};
};

}

}