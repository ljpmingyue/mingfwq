#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include "scheduler.h"
#include "fd_manager.h"
#include "log.h"

#include <dlfcn.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>

    static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");

namespace mingfwq{
    //ThreadLocal在一个线程中是共享的，在不同线程之间又是隔离的
    static thread_local bool t_hook_enable = false;

    static mingfwq::ConfigVar<int>::ptr g_tcp_connect_timeout =
        mingfwq::Config::Lookup("tcp.connect.timeout",5000,"tcp connect timeout");

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep)\
    XX(nanosleep)\
    XX(socket)\
    XX(connect)\
    XX(accept)\
    XX(read)\
    XX(readv)\
    XX(recv)\
    XX(recvfrom)\
    XX(recvmsg)\
    XX(write)\
    XX(writev)\
    XX(send)\
    XX(sendto)\
    XX(sendmsg)\
    XX(close)\
    XX(fcntl)\
    XX(ioctl)\
    XX(getsockopt)\
    XX(setsockopt)

    void hook_init(){
        static bool is_inited = false;
        if(is_inited){
            return;
        }
//dlsym 动态库里取函数的方法
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
#undef XX
    }

    static uint64_t s_connect_timeout = -1;

struct _HookIniter{
    _HookIniter(){
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value,const int& new_value){
            MINGFWQ_LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value
                                        << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};
 
    static _HookIniter s_hook_init;


    bool is_hook_enable(){
        return t_hook_enable;
    }

    void set_hook_enable(bool flag){
        t_hook_enable = flag;
    }

}


//定时器的情况
struct timer_info{
    int cancelled = 0;
};

    template<typename OriginFun,typename ... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
            uint32_t event, int timeout_so, Args&&... args){
        if(!mingfwq::t_hook_enable){
            return fun(fd, std::forward<Args>(args)...);
        }
        //MINGFWQ_LOG_INFO(g_logger) << "do_io <" << hook_fun_name << ">";

        mingfwq::FdCtx::ptr ctx = mingfwq::FdMgr::GetInstance()->get(fd);
        if(!ctx){
            return fun(fd, std::forward<Args>(args)...);
        }

        if(ctx->isClose()){
            errno = EBADF;
            return -1;
        }

        if(!ctx->isSocket() || ctx->getUserNonblock()){
            return fun(fd, std::forward<Args>(args)...);
        }
        //判断他是哪种超时时间
        uint64_t to = ctx->getTimeout(timeout_so);
        //超时的条件或者说超时的原因
        std::shared_ptr<timer_info> tinfo(new timer_info);

    retry:
        //开始读入数据
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        while (n == -1 && errno == EINTR)
        {   //系统调用会被中断，需要人为重启
            ////错误为EINTR：捕获到某个信号且相应信号处理函数返回时，这个系统调用被中断
            n = fun(fd, std::forward<Args>(args)...);
        }
        if(n == -1 && errno == EAGAIN){
            /* errno = EAGAIN
                你连续做read操作而没有数据可读。此时程序不会阻塞起来等待数据准备就绪返回，
            read函数会返回一个错误EAGAIN，提示你的应用程序现在没有数据可读请稍后再试*/
            mingfwq::IOManager* iom = mingfwq::IOManager::GetThis();
            mingfwq::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);

            if(to != (uint64_t)-1 ){
                timer = iom->addConditionTimer(to,[winfo, fd, iom, event](){
                    auto t = winfo.lock();
                    if(!t || t->cancelled){
                        return;
                    }
                    //定时器状态设置成超时，然后强制触发它
                    t->cancelled = ETIMEDOUT;
                    //iom->cancelEvent(fd, event);
                    iom->cancelEvent(fd, (mingfwq::IOManager::Event)event);
                }, winfo);
            }

            //这里添加事件，没带回调函数，默认用当前的协程做回调
            int rt = iom->addEvent(fd, (mingfwq::IOManager::Event)event); 
            if(MINGFWQ_UNLICKLY(rt)){ //添加定时器出错了，进入这里面
                MINGFWQ_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                    << fd << ", "<< event << ") ";
                if(timer){
                    timer->cancel();
                }
                return -1;

            }else{
                //MINGFWQ_LOG_INFO(g_logger) << "do_io YieldToHold before";
                mingfwq::Fiber::YieldToHold();
                //MINGFWQ_LOG_INFO(g_logger) << "do_io YieldToHold after";
                if(timer){
                    timer->cancel();
                }
                if(tinfo->cancelled){
                    errno = tinfo->cancelled;
                    return -1;
                }

                goto retry;
            }
            

        }

        return n;

    }



extern "C"{
//声名一下函数
#define XX(name) name ## _fun name ## _f = nullptr; 
    HOOK_FUN(XX);
#undef XX

    unsigned int sleep(unsigned int seconds){
        if(!mingfwq::t_hook_enable){
            return sleep_f(seconds);
        }
        mingfwq::Fiber::ptr fiber = mingfwq::Fiber::GetThis();
        mingfwq::IOManager* iom = mingfwq::IOManager::GetThis();
        iom->addTimer(seconds *  1000, std::bind((void(mingfwq::Scheduler::*)
                            (mingfwq::Fiber::ptr, int thread))&mingfwq::IOManager::schedule,
                            iom,fiber,-1));
        /*
        iom->addTimer(seconds * 1000, [fiber,iom](){
            iom->schedule(fiber);
        });*/
        mingfwq::Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec){
        if(!mingfwq::t_hook_enable){
            return usleep_f(usec);
        }
        mingfwq::Fiber::ptr fiber = mingfwq::Fiber::GetThis();
        mingfwq::IOManager* iom = mingfwq::IOManager::GetThis();
        iom->addTimer(usec /  1000, std::bind((void(mingfwq::Scheduler::*)
                            (mingfwq::Fiber::ptr, int thread))&mingfwq::IOManager::schedule,
                            iom,fiber,-1));
        /*        
        iom->addTimer(usec /  1000, [fiber,iom](){
            iom->schedule(fiber);
        });*/
        mingfwq::Fiber::YieldToHold();
        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem){
        if(!mingfwq::t_hook_enable){
            return nanosleep_f(req, rem);
        }
        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec /1000/1000;
        mingfwq::Fiber::ptr fiber = mingfwq::Fiber::GetThis();
        mingfwq::IOManager* iom = mingfwq::IOManager::GetThis();
        iom->addTimer(timeout_ms, std::bind((void(mingfwq::Scheduler::*)
                            (mingfwq::Fiber::ptr, int thread))&mingfwq::IOManager::schedule,
                            iom,fiber,-1));
        /*
        iom->addTimer(timeout_ms, [fiber,iom](){
            iom->schedule(fiber);
        });*/
        mingfwq::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol){
        if(!mingfwq::t_hook_enable){
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        if(fd == -1){
            return fd;
        }
        mingfwq::FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,uint64_t timeout_ms){
        if(!mingfwq::t_hook_enable){
            return connect_f(fd,addr,addrlen);
        }
        mingfwq::FdCtx::ptr ctx = mingfwq::FdMgr::GetInstance()->get(fd);
        if(!ctx || ctx->isClose()){
            errno = EBADF;
            return -1;
        }
        if(!ctx->isSocket()){
            return connect_f(fd,addr,addrlen);
        }
        if(ctx->getUserNonblock()){
            return connect_f(fd,addr,addrlen);
        }
        int n = connect_f(fd,addr,addrlen);
        if(n == 0){
            return 0;
        }else if(n != -1 || errno != EINPROGRESS){
            return n;
        }

        mingfwq::IOManager* iom = mingfwq::IOManager::GetThis();
        mingfwq::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);

        if(timeout_ms != (uint64_t)-1){
            timer = iom->addConditionTimer(timeout_ms, [winfo,iom,fd](){
                auto t = winfo.lock();
                if(!t || t->cancelled){
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd,mingfwq::IOManager::WRITE);

            },winfo);
        }
        //添加完定时器就开始添加任务
        int rt = iom->addEvent(fd,mingfwq::IOManager::WRITE);
        if(rt == 0){
            mingfwq::Fiber::YieldToHold();
            if(timer){
                timer->cancel();
            }
            if(tinfo->cancelled){
                errno = tinfo->cancelled;
                return -1;
            }

        }else {
            if(timer){
                timer->cancel();
            }
            MINGFWQ_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
        }

        //与其他socket不同，唤醒之后不需要重新回去读数据，唤醒之后不是超时了，就是已经连接成功了
        int error = 0;
        socklen_t len = sizeof(int);
        if(-1 == getsockopt_f(fd,SOL_SOCKET, SO_ERROR, &error,&len)){
            return -1;
        }
        if(!error){
            return 0;
        }else{
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
        return connect_with_timeout(sockfd,addr,addrlen, mingfwq::s_connect_timeout);
    }

    int accept(int s, struct sockaddr *addr, socklen_t *addrlen){
        int fd = do_io(s, accept_f, "accept", mingfwq::IOManager::READ, SO_RCVTIMEO, addr,addrlen);
        if(fd >= 0){
            mingfwq::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    ssize_t read(int fd, void *buf, size_t count){
        return do_io(fd, read_f, "read", mingfwq::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
        return do_io(fd, readv_f, "readv", mingfwq::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags){
        return do_io(sockfd, recv_f, "recv", mingfwq::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                    struct sockaddr *src_addr, socklen_t *addrlen){
        return do_io(sockfd, recvfrom_f, "recvform", mingfwq::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr,addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
        return do_io(sockfd, recvmsg_f, "recvmsg", mingfwq::IOManager::READ, SO_RCVTIMEO,msg, flags);
    }

    ssize_t write(int fd, const void *buf, size_t count){
        return do_io(fd, write_f, "write", mingfwq::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
        return do_io(fd, writev_f, "writev", mingfwq::IOManager::WRITE, SO_SNDTIMEO,iov,iovcnt);
    }

    ssize_t  send(int s, const void *msg, size_t len, int flags){
        return do_io(s, send_f, "send", mingfwq::IOManager::WRITE, SO_SNDTIMEO,msg,len,flags);
    }   

    ssize_t  sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){
        return do_io(s, sendto_f, "sendto", mingfwq::IOManager::WRITE, SO_SNDTIMEO,msg,len,flags,to,tolen);
    }

    ssize_t  sendmsg(int s, const struct msghdr *msg, int flags){
        return do_io(s, sendmsg_f, "sendmsg", mingfwq::IOManager::WRITE, SO_SNDTIMEO,msg,flags);
    }

    int close(int fd){
        if(mingfwq::t_hook_enable){
            return close_f(fd);
        }
        mingfwq::FdCtx::ptr ctx = mingfwq::FdMgr::GetInstance()->get(fd);
        if(!ctx){
            auto iom = mingfwq::IOManager::GetThis();
            if(iom){
                iom->cancelAll(fd);
            }
            mingfwq::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }


    int fcntl(int fd, int cmd, .../* arg */ ){
        /*  是否hook都不能这么写，...放不进去
        if(!mingfwq::t_hook_enable){
            return fcntl_f(fd, cmd,...);
        }
        */
        va_list va;
        va_start(va,cmd);
        switch (cmd){
        case F_SETFL:
            {
                int arg = va_arg(va,int);
                va_end(va);
                mingfwq::FdCtx::ptr ctx = mingfwq::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()){
                    return fcntl_f(fd,cmd,arg);
                }
                //查看是不是用户设置过了nonblock
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()){
                    arg |= O_NONBLOCK;
                }else{
                    //如果不是就删掉O_NONBLOCK
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd,cmd,arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd,cmd);
                mingfwq::FdCtx::ptr ctx = mingfwq::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || ctx->isSocket()){
                    return arg;
                }
                if(ctx->getUserNonblock()){
                    return arg | O_NONBLOCK;
                }else{
                    return arg & ~O_NONBLOCK;
                }
            }
            break;      
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd,cmd, arg);
            }
            break;

        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd,cmd);
            }
            break;

        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd,cmd,arg);
            }
            break;

        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_ex* arg = va_arg(va,struct f_owner_ex*);
                va_end(va);
                return fcntl_f(fd,cmd,arg);
            }
            break;
        
        default:
            va_end(va);
            return fcntl_f(fd,cmd);
            break;
        }
    }

    int ioctl(int fd, unsigned long request, ...){
        va_list va;
        va_start(va,request);
        void* arg = va_arg(va,void*);
        va_end(va);
        if(FIONBIO == request){
            bool user_nonblock = !!*(int*)arg;
            mingfwq::FdCtx::ptr ctx = mingfwq::FdMgr::GetInstance()->get(fd);
            if(!ctx || ctx->isClose() || ctx->isSocket()){
                return ioctl_f(fd,request,arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(fd,request,arg);
    }
    /*  sockfd:将要被设置或获取选项的套接字
        level--选项级别（选项所在的协议层）
                SOL_SOCKET：通用socket选项
                IPPROTO_IP：IP选项
                IPPROTO_TCP：TCP选项
        optname--选项名称（需要访问的选项名）
                每一项level下方都有不同得optname
        optval--选项值（对于getsockopt，指向返回选项值的缓冲；
                对于setsockopt，指向包含新选项值的缓冲）
        optlen--选项值的长度/存放选项值长度的指针
    **
    */

    int getsockopt(int sockfd, int level, int optname,
                    void *optval, socklen_t *optlen){
        return getsockopt_f(sockfd,level,optname,optval,optlen);
    }

    int setsockopt(int sockfd, int level, int optname,
                    const void *optval, socklen_t optlen){
        if(!mingfwq::t_hook_enable){
            return setsockopt_f(sockfd,level,optname,optval,optlen);
        }
        if(level == SOL_SOCKET){
            if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO){
                mingfwq::FdCtx::ptr ctx = mingfwq::FdMgr::GetInstance()->get(sockfd);
                if(ctx){    
                    const timeval* v = (const timeval*)optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd,level,optname,optval,optlen);
    }



}

