#include "http_connection.h"
#include "http_parser.h"
#include "../log.h"
#include "../socket.h"

namespace mingfwq{
namespace http{
    static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");

    std::string HttpResult::toString()const{
        std::stringstream ss;
        ss << "[HttpResult reslut = " << result
            << ",error = " << error
            << ",response" << (response? response->toString():nullptr);
        return ss.str();
    }

    HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
            :SocketStream(sock,owner){
    }

    HttpConnection::~HttpConnection(){
        MINGFWQ_LOG_INFO(g_logger) << "HttpConnection::~HttpConnection";
    }
    
    HttpResult::ptr mingfwq::http::HttpConnection::DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string,std::string>& headers 
                            , const std::string& body ){
        Uri::ptr uri = Uri::Create(url);
        if(!uri){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                        ,nullptr, "invalid url: " + url);
        }
        return DoGet(uri,timeout_ms,headers,body);
    }

    HttpResult::ptr mingfwq::http::HttpConnection::DoGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string,std::string>& headers 
                            , const std::string& body ){
        return DoRequest(HttpMethod::GET,uri,timeout_ms,headers,body);
    }

    HttpResult::ptr mingfwq::http::HttpConnection::DoPost(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string,std::string>& headers
                            , const std::string& body ){
        Uri::ptr uri = Uri::Create(url);
        if(!uri){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                        ,nullptr, "invalid url: " + url);
        }
        return DoPost(uri,timeout_ms,headers,body);
    }

    HttpResult::ptr mingfwq::http::HttpConnection::DoPost(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string,std::string>& headers
                            , const std::string& body ){
        return DoRequest(HttpMethod::POST,uri,timeout_ms,headers,body);
    }

    HttpResult::ptr mingfwq::http::HttpConnection::DoRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string,std::string>& headers
                            , const std::string& body){
        Uri::ptr uri = Uri::Create(url);
        if(!uri){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                        ,nullptr, "invalid url: " + url);
        }
        return DoRequest(method,uri,timeout_ms,headers,body);
    }

    HttpResult::ptr mingfwq::http::HttpConnection::DoRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string,std::string>& headers
                            , const std::string& body ){
        HttpRequest::ptr req = std::make_shared<HttpRequest>();
        req->setPath(uri->getPath());
        req->setQuery(uri->getQuery());
        req->setFragment(uri->getFragment());
        req->setMethod(method);
        bool has_host = false;
        for(auto& i : headers){
            //如果有连接 connection
            if(strcasecmp(i.first.c_str(), "connection") == 0){
                if(strcasecmp(i.first.c_str(), "keep-alive") == 0){
                    req->setClose(false);
                }
                continue;
            }
            if( !has_host ||strcasecmp(i.first.c_str(), "host") == 0){
                has_host = !i.second.empty();
            }
            req->setHeader(i.first,i.second); 
        }
        if(!has_host){
            req->setHeader("Host", uri->getHost());
        }
        req->setBody(body);

        return DoRequest(req, uri, timeout_ms);
    }

    HttpResult::ptr mingfwq::http::HttpConnection::DoRequest(HttpRequest::ptr req
                            , Uri::ptr uri
                            , uint64_t timeout_ms){
        Address::ptr addr = uri->createAddress();
        if(!addr){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST,
                                nullptr,"invalid host: "+ uri->getHost());
        }
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock){
            return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR,
                                nullptr,"connect fail: " + addr->toString());
        }
        if(!sock->connect(addr)){
            return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL,
                                nullptr,"connect fail: " + addr->toString());
        }
        sock->setRecvTimeout(timeout_ms);
        HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
        int rt = conn->sendRequest(req);
        if(rt == 0){
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER,
                                nullptr,"send request closed  by peer: " + addr->toString());
        }else if(rt < 0){
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR,
                                nullptr,"send request socket error,errno = " + std::to_string(errno)
                                 + ",strerr = " + strerror(errno));
        }
        auto res = conn->recvResponse();
        if(!res){
            return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT,nullptr,
                                "timeout:" + std::to_string(timeout_ms) + ",GetrecvTimeout:" + std::to_string(sock->getRecvTinmeout()));
        }

        return std::make_shared<HttpResult>((int)HttpResult::Error::OK, res ,"OK");
    }

    HttpResponse::ptr HttpConnection::recvResponse(){
        HttpResponseParser::ptr parser(new HttpResponseParser);
        uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
        //uint64_t buff_size = 150;
        
        //智能指针只会当他是一个成员只会delete，但其实是个数组
        std::shared_ptr<char> buffer(
            new char[buff_size + 1], [](char* ptr){
                delete[] ptr;
            });
        
        char* data = buffer.get();
        int offset = 0;
        do{
            int len = read(data + offset, buff_size - offset);
            if(len <= 0){
                close();
                return nullptr;
            }
            //-----------------
            len += offset;
            data[len] = '\0';
            //返回解析长度
            size_t nparesr = parser->execute(data, len, true);

            if(parser->hasError()){
                close();
                return nullptr;
            }
            offset = len - nparesr;

            if(offset == (int)buff_size){
                close();
                return nullptr;
            }
            if(parser->isFinished()){
                break;
            }
        }while (true);

        auto& client_parser = parser->getParser();
        std::string body;
        //是不是分段的如果是chunked就为1
        if(client_parser.chunked){
            int len = offset;
            do{
                do{ //分片的话就一直读数据读到退出nullptr 或者全部读出来isFinished返回为0
                    int rt = read(data + len, buff_size - len);
                    if(rt <= 0){
                        close();
                        return nullptr;
                    }
                    len += rt;
                    data[len] = '\0';
                    size_t nparse = parser->execute(data,len,true);
                    if(parser->hasError()){
                        close();
                        return nullptr;
                    }
                    len -= nparse;
                    //如果读了数据却解析不了就退出去
                    if(len == (int)buff_size){
                        close();
                        return nullptr;
                    }
                    
                }while(!parser->isFinished());

                len -= 2;
                MINGFWQ_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;
                //如果解析器的内容长度 小于读取到的内容长度
                if(client_parser.content_len <= len){
                    body.append(data, client_parser.content_len);
                    memmove(data, data + client_parser.content_len
                                    , len - client_parser.content_len);
                    len -= client_parser.content_len;
                }else{
                    body.append(data, len);
                    int left = client_parser.content_len - len;
                    while (left > 0){
                        int rt = read(data, left > (int)buff_size? (int)buff_size: left);
                        if(rt <= 0){
                            close();
                            return nullptr;
                        }
                        body.append(data, rt);
                        left -= rt;
                    }
                    len = 0;
                }
            }while(client_parser.chunks_done);
            parser->getData()->setBody(body);

        }else{//不是分段

            //用int64_t 不用uint64_t是在减法操作时候不容易溢出
            int64_t length = parser->getContentLength();
            if(length > 0){
                std::string body;
                body.resize(length);

                int len = 0;
                if(length >= offset){
                    //body.append(data,offset);
                    memcpy(&body[0], data, offset);
                    len = offset;
                }else{
                    //body.append(data,length);
                    memcpy(&body[0], data, length);
                    len = length;
                }
                length -= offset;
                if(length > 0){
                    //if(readFixSize(&body[body.size()],length) <= 0){
                    if(readFixSize(&body[len],length) <= 0){
                        close();
                        return nullptr;
                    }
                    parser->getData()->setBody(body);
                }  
            }
        }
        return parser->getData();
    }

    int HttpConnection::sendRequest(HttpRequest::ptr req){
        std::stringstream ss;
        ss << *req;
        std::string data = ss.str();
        return writeFixSize(data.c_str(),data.size());
    }


    //HttpConnectionPool
    HttpConnectionPool::HttpConnectionPool(const std::string& host
                        ,const std::string& vhost
                        ,uint32_t port
                        ,uint32_t max_size
                        ,uint32_t max_alive_time
                        ,uint32_t max_request)
            :m_host(host)
            ,m_vhost(vhost)
            ,m_port(port)
            ,m_maxSize(max_size)
            ,m_maxAliveTime(max_alive_time)
            ,m_maxRequest(max_request){

    }
    
    HttpConnection::ptr HttpConnectionPool::getConnection(){
        uint64_t now_ms = mingfwq::GetCurrentMS();
        std::vector<HttpConnection*> invalid_conns;
        HttpConnection* ptr = nullptr;
        MutexType::Lock lock(m_mutex);
        while (!m_conns.empty()){
            auto conn = *m_conns.begin();
            if(!conn->isConnected()){
                invalid_conns.push_back(conn);
                continue;
            }
            if((conn->m_createTime + (uint64_t)m_maxAliveTime) > now_ms){
                invalid_conns.push_back(conn);
                continue;
            }
            m_conns.pop_front();
            ptr = conn;
            break;
        }
        lock.unlock();
        
        for(auto i : invalid_conns){
            delete i;
        }
        m_total -= invalid_conns.size();
        
        if(!ptr){
            IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
            if(!addr){
                MINGFWQ_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
                return nullptr;
            }
            addr->setPort(m_port);
            Socket::ptr sock = Socket::CreateTCP(addr);
            if(!sock){
                MINGFWQ_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
                return nullptr;
            }
            if(!sock->connect(addr)){
                MINGFWQ_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
                return nullptr;
            }
            ptr = new HttpConnection(sock);
            ++m_total;
        }
        return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr
                            ,std::placeholders::_1, this));
        
    }

    void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool){
        ++(ptr->m_request);
        if(!ptr->isConnected()
                    || (ptr->m_createTime + pool->m_maxAliveTime) >= mingfwq::GetCurrentMS()
                    || ((ptr->m_request) > (uint64_t)(pool->m_maxRequest))) {
            delete ptr;
            --(pool->m_total);
            return;
        }
        
        MutexType::Lock lock(pool->m_mutex);
        pool->m_conns.push_back(ptr);
    }

    HttpResult::ptr HttpConnectionPool::doGet(const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers
                        , const std::string& body ){
        return doRequest(HttpMethod::GET,url,timeout_ms,headers,body);
    }

    HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers 
                        , const std::string& body ){
        std::stringstream ss;
        ss  << uri->getPath()
            << (uri->getQuery().empty()? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty()? "" : "#")
            << uri->getFragment();
        return doGet(ss.str(),timeout_ms,headers,body);
    }

    HttpResult::ptr HttpConnectionPool::doPost(const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers
                        , const std::string& body){
        return doRequest(HttpMethod::POST,url,timeout_ms,headers,body);
    }

    HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers
                        , const std::string& body ){
        std::stringstream ss;
        ss  << uri->getPath()
            << (uri->getQuery().empty()? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty()? "" : "#")
            << uri->getFragment();
        return doPost(ss.str(),timeout_ms,headers,body);
    }

    HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                        , const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers
                        , const std::string& body ){
        HttpRequest::ptr req = std::make_shared<HttpRequest>();
        req->setPath(url);
        req->setMethod(method);
        req->setClose(false);
        bool has_host = false;
        for(auto& i : headers){
            //如果有连接 connection
            if(strcasecmp(i.first.c_str(), "connection") == 0){
                if(strcasecmp(i.first.c_str(), "keep-alive") == 0){
                    req->setClose(false);
                }
                continue;
            }
            if( !has_host ||strcasecmp(i.first.c_str(), "host") == 0){
                has_host = !i.second.empty();
            }
            req->setHeader(i.first,i.second); 
        }
        if(!has_host){
            if(m_vhost.empty()){
                req->setHeader("host",m_host);
            }else{
                req->setHeader("host",m_vhost);
            }
        }
        req->setBody(body);

        return doRequest(req, timeout_ms);
    }
    
    HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                        , Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string,std::string>& headers
                        , const std::string& body){
        std::stringstream ss;
        ss  << uri->getPath()
            << (uri->getQuery().empty()? "" : "?")
            << uri->getQuery()
            << (uri->getFragment().empty()? "" : "#")
            << uri->getFragment();
        return doRequest(method,ss.str(),timeout_ms,headers,body);
    }

    HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
                        , uint64_t timeout_ms){
        auto conn = getConnection();
        if(!conn){
            return std::make_shared<HttpResult>((int)mingfwq::http::HttpResult::Error::POOL_GET_CONNECTION
                            ,nullptr,"pool get connection fail:" + m_host + ",port:" + std::to_string(m_port));
        }
        
        auto sock = conn->getSocket();
        if(!sock){
            return std::make_shared<HttpResult>((int)mingfwq::http::HttpResult::Error::POOL_INVALID_CONNECTION
                            ,nullptr,"pool get connection fail:" + m_host + ",port:" + std::to_string(m_port));
        }
        auto addr = sock->getRemoteAddress();
        //后面连接部分与前面HttpConnection::DoRequest后一部分一样
        sock->setRecvTimeout(timeout_ms);
        int rt = conn->sendRequest(req);
        if(rt == 0){
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER,
                                nullptr,"send request closed  by peer: " + addr->toString());
        }else if(rt < 0){
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR,
                                nullptr,"send request socket error,errno = " + std::to_string(errno)
                                 + ",strerr = " + strerror(errno));
        }
        auto res = conn->recvResponse();
        if(!res){
            return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT,nullptr,
                                "timeout:" + std::to_string(timeout_ms) + ",GetrecvTimeout:" + std::to_string(sock->getRecvTinmeout()));
        }

        return std::make_shared<HttpResult>((int)HttpResult::Error::OK, res ,"OK");
    }




}


}