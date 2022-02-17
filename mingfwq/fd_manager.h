#pragma once
#include "mingfwq.h"

#include <memory>
#include <vector>

namespace mingfwq{

//管理文件句柄类型(是否socket), 是否阻塞, 是否关闭, 读/写超时时间
class FdCtx:public std::enable_shared_from_this<FdCtx>{
public:
    typedef std::shared_ptr<FdCtx> ptr;
    typedef Mutex MutexType;

    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool isInit()const{return m_isInit;}
    bool isSocket()const{return m_isSocket;}
    bool isClose()const{return m_isClosed;}

    void setUserNonblock(bool v){ m_userNonblock = v;}
    bool getUserNonblock()const{ return m_userNonblock;}

    void setSysNonblock(bool v){ m_userNonblock = v;}
    bool getSysNonblock()const{return m_userNonblock;}

    //用读写来区别是那种超时
    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);


private:
    //是否初始化
    bool m_isInit: 1;
    //是否socket
    bool m_isSocket: 1;
    //是否hook非阻塞
    bool m_sysNonblock: 1;
    //是否用户主动设置非阻塞
    bool m_userNonblock: 1;
    //是否关闭
    bool m_isClosed: 1;

    int m_fd;
    //读超时时间毫秒
    uint64_t m_recvTimeout;
    //写超时时间毫秒
    uint64_t m_sendTimeout;

};

class FdManager{
public:
    typedef std::shared_ptr<FdManager> ptr;
    typedef RWMutex RWMutexType;

    FdManager();

    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

}