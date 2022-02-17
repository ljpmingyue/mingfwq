#pragma once 
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "../thread.h"

namespace mingfwq{
namespace http{
class Servlet{
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string& name)
            :m_name(name){}
    virtual ~Servlet(){}
    //真正处理请求
    virtual int32_t handle(mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr response
                ,mingfwq::http::HttpSession::ptr session) = 0;
    const std::string& getName()const{return m_name;}
protected:
    std::string m_name;
};

//函数式Servlet
class FunctionServlet:public Servlet{
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr response
                ,mingfwq::http::HttpSession::ptr session)> callback;
public:
    FunctionServlet(callback cb);
    int32_t handle(mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr HttpResponse
                ,mingfwq::http::HttpSession::ptr session)override;
private:
    callback m_cb;
};


//Servlet分发器
class ServletDispath:public Servlet{
public:
    typedef std::shared_ptr<ServletDispath> ptr;
    typedef mingfwq::RWMutex RWMutexType;

public:
    ServletDispath();
    int32_t handle(mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr response
                ,mingfwq::http::HttpSession::ptr session)override;

    //添加servlet(精准匹配)Servlet::ptr
    void addServlet(const std::string& uri, Servlet::ptr slt);
    //添加servlet(精准匹配)FunctionServlet::callback
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    //添加模糊匹配 Servlet::ptr
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    //添加模糊匹配 FunctionServlet::callback
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const  std::string& uri);
    void delGlobServlet(const  std::string& uri);

    Servlet::ptr getDefault()const{return m_default;}
    void setDefault(Servlet::ptr v){m_default = v;};

    //查找范围m_datas
    Servlet::ptr getServlet(const std::string& uri);
    //查找范围m_globs
    Servlet::ptr getGlobServlet(const std::string& uri);
    //查找范围m_datas,m_globs,m_default
    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    //uri(/mingfwq/xxx) --> servlet   精准匹配
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    //uri(/mingfwq/*) --> servlet   模糊匹配
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
    //默认servlet, 所有路径都没有匹配到的时候用到
    Servlet::ptr m_default;

    RWMutexType m_mutex;
};

//
class NotFoundServlet:public Servlet{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet();
    int32_t handle(mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr response
                ,mingfwq::http::HttpSession::ptr session)override;
};


}
}
