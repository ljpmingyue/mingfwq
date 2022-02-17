#include "servlet.h"
#include <fnmatch.h>

namespace mingfwq{
namespace http{
    //FunctionServlet
    FunctionServlet::FunctionServlet(callback cb)
                :Servlet("FunctionServlet")
                ,m_cb(cb){

    }

    int32_t FunctionServlet::handle(mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr response
                ,mingfwq::http::HttpSession::ptr session){
        return m_cb(request,response,session);
    }

    //ServletDispath
    ServletDispath::ServletDispath()
                :Servlet("ServletDispath"){
        m_default.reset(new NotFoundServlet());
    }

    int32_t ServletDispath::handle(mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr response
                ,mingfwq::http::HttpSession::ptr session){
        Servlet::ptr it =  getMatchedServlet(request->getPath());
        if(it){
            it->handle(request,response,session);
        }
        return 0;
    }

    void ServletDispath::addServlet(const std::string& uri, Servlet::ptr slt){
        RWMutexType::WriteLock lock(m_mutex);
        m_datas[uri] = slt;
    }

    void ServletDispath::addServlet(const std::string& uri, FunctionServlet::callback cb){
        RWMutexType::WriteLock lock(m_mutex);
        m_datas[uri].reset(new FunctionServlet(cb));
    }

    void ServletDispath::addGlobServlet(const std::string& uri, Servlet::ptr slt){
        RWMutexType::WriteLock lock(m_mutex);
        for(auto it = m_globs.begin();it != m_globs.end(); ++it){
            // fnmatch 可以用作模糊匹配 匹配*号
            if(!fnmatch(it->first.c_str(),uri.c_str(),0)){
                m_globs.erase(it);
                break;
            }
        }
        m_globs.push_back(std::make_pair(uri,slt));
    }

    void ServletDispath::addGlobServlet(const std::string& uri, FunctionServlet::callback cb){
        return addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
    }

    void ServletDispath::delServlet(const  std::string& uri){
        RWMutexType::WriteLock lock(m_mutex);
        m_datas.erase(uri);
    }

    void ServletDispath::delGlobServlet(const  std::string& uri){
        RWMutexType::WriteLock lock(m_mutex);
        for(auto it = m_globs.begin();it != m_globs.end(); ++it){
            // fnmatch 可以用作模糊匹配 匹配*号
            if(!fnmatch(it->first.c_str(),uri.c_str(),0)){
                m_globs.erase(it);
                break;
            }
        }
    }

    Servlet::ptr ServletDispath::getServlet(const std::string& uri){
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_datas.find(uri);
        return it == m_datas.end() ? nullptr: it->second;
    }

    Servlet::ptr ServletDispath::getGlobServlet(const std::string& uri){
        RWMutexType::ReadLock lock(m_mutex);
        for(auto it = m_globs.begin(); it != m_globs.end(); ++it){
            if(uri == it->first){
                return it->second;
            }
        }
        return nullptr;
    }

    Servlet::ptr ServletDispath::getMatchedServlet(const std::string& uri){
        RWMutexType::ReadLock lock(m_mutex);
        auto ait = m_datas.find(uri);
        if(ait != m_datas.end()){
            return ait->second;
        }
        for(auto it = m_globs.begin(); it != m_globs.end(); ++it){
            if(uri == it->first){
                return it->second;
            }
        }
        return m_default;
    }

    //NotFoundServlet
    NotFoundServlet::NotFoundServlet()
            :Servlet("NotFoundServlet"){

    }

    int32_t NotFoundServlet::handle(mingfwq::http::HttpRequest::ptr request
                ,mingfwq::http::HttpResponse::ptr response
                ,mingfwq::http::HttpSession::ptr session){
        static const std::string& RSP_BODY ="<html><head><title>404 Not Found"
                    "</title></head><body><center><h1>404 Not Found</h1></center>"
                    "<hr><center>mingfwq/1.0.0</center></body></html>";
        
        response->setStatus(mingfwq::http::HttpStatus::NOT_FOUND);
        response->setHeader("Server","mingfwq/1.0.0");
        response->setHeader("Content-Type","text/html");
        response->setBody(RSP_BODY);

        return 0;
    }
}
}