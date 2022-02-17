#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace mingfwq{

    static Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");


    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event){
        switch (event) {
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                MINGFWQ_ASSERT2(false, "getContext");
        }
    }

    void IOManager::FdContext::resetContext(EventContext& ctx){
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }
    
    void IOManager::FdContext::triggerEvent(Event event){
        MINGFWQ_ASSERT(events & event);
        //将events删掉
        events = (Event)(events & ~event);
        EventContext& ctx = getContext(event);
        if(ctx.cb){
            ctx.scheduler->schedule(&ctx.cb);
        }else {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr;
        return;

    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
                :Scheduler(threads,use_caller,name){
        m_epfd = epoll_create(5000);
        MINGFWQ_ASSERT(m_epfd > 0);

        //int pipe(int pipefd[2]); 成功：0；失败：-1，设置errno
        int rt = pipe(m_tickleFds);
        MINGFWQ_ASSERT(!rt);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];

        //
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        MINGFWQ_ASSERT(!rt);  
        //添加事件
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        MINGFWQ_ASSERT(!rt);  

        //m_FdContexts.resize(64);
        contextResize(32);

\
        start();
    }

    IOManager::~IOManager(){
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for(size_t i = 0; i < m_FdContexts.size(); ++i){
            if(m_FdContexts[i]){
                delete m_FdContexts[i];
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void ()> cb){
         
        FdContext* fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if((int)m_FdContexts.size() > fd ){
            fd_ctx = m_FdContexts[fd];
            lock.unlock();
        } else{
            lock.unlock();
            RWMutexType::WriteLock lock(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_FdContexts[fd];
        }

        MutexType::Lock lock2(fd_ctx->mutex);
        if(fd_ctx->events & event){
            //进入这里则说明同一个句柄被再加一次，两个不同的线程加载同一个句柄
            MINGFWQ_LOG_DEBUG(g_logger) << "addEvent assert fd" << fd
                                        << " event" << event
                                        << " fd_ctx.event" << fd_ctx->events;
            MINGFWQ_ASSERT(!(fd_ctx->events & event));
        }

        int op = fd_ctx->events ? EPOLL_CTL_MOD: EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;
        /*
            将被监听的描述符添加到epoll句柄或从epool句柄中删除或者对监听事件进行修改
            epfd:由epoll_create生成的epoll的专用文件描述符
            op: 要进行的操作，如 EPOLL_CTL_ADD 注册；EPOLL_CTL_MOD 修改；EPPOL_CTL_DEL 删除
            fd: 要关联的文件描述符
            event: 指向epoll_event的指针
        */
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
         
        if(rt){     //如果添加失败了
            MINGFWQ_LOG_DEBUG(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd << ", " << epevent.events << "):"
                << rt << " (" << errno << ")(" << strerror(errno) << ")";
            return -1;
        }

        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event);
        //从事件上下文类中取出事件，如果有值说明上面有事件了就会报错
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        MINGFWQ_ASSERT( !event_ctx.scheduler 
                     && !event_ctx.fiber 
                     && !event_ctx.cb);
        
        event_ctx.scheduler = Scheduler::GetThis();
        if(cb){
            event_ctx.cb.swap(cb);
        } else{
            event_ctx.fiber = Fiber::GetThis();
            MINGFWQ_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                            "state = " << event_ctx.fiber->getState());
        }

        //MINGFWQ_LOG_INFO(g_logger) << "addEvent end";
        return 0;
        
    }

    void IOManager::contextResize(size_t size){
        m_FdContexts.resize(size);
        for(size_t i = 0; i < m_FdContexts.size(); ++i){
            if(!m_FdContexts[i]){
                m_FdContexts[i] = new FdContext;
                m_FdContexts[i]->fd = i;
             }
        }
    }

    bool IOManager::delEvent(int fd, Event event){
        RWMutexType::WriteLock lock(m_mutex);
        if((int)m_FdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_FdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if( !(fd_ctx->events & event) ){
            return false;
        }

        //比较fd里面的events和输入的一不一样，
        //不一样则是将原来的修改成新的，一样则直接删除前面的
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD: EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;
        
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt){     //如果删除失败了
            MINGFWQ_LOG_DEBUG(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd << ", " << epevent.events << "):"
                << rt << " (" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        -- m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);

        return true;


    }

    bool IOManager::cancelEvent(int fd, Event event){
        RWMutexType::WriteLock lock(m_mutex);
        if((int)m_FdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_FdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if( !(fd_ctx->events & event) ){
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD: EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;
        
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt){     //如果删除失败了
            MINGFWQ_LOG_DEBUG(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd << ", " << epevent.events << "):"
                << rt << " (" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        //存在就触发它
        fd_ctx->triggerEvent (event);
        --m_pendingEventCount;


        return true;
    }

    bool IOManager::cancelAll(int fd){
        RWMutexType::WriteLock lock(m_mutex);
        if((int)m_FdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_FdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if( !fd_ctx->events ){
            return false;
        }

        
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;
        
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt){     //如果删除失败了
            MINGFWQ_LOG_DEBUG(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd << ", " << epevent.events << "):"
                << rt << " (" << errno << ")(" << strerror(errno) << ")";
            return false;
        }


        if(fd_ctx->events & READ){
            fd_ctx->triggerEvent (READ);
            --m_pendingEventCount;
        }
        if(fd_ctx->events & WRITE){
            fd_ctx->triggerEvent (WRITE);
            --m_pendingEventCount;
        }
        
        MINGFWQ_ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager* IOManager::GetThis(){
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }

    void IOManager::tickle(){
        if( !hasIdleThreads() ){
            return;
        }
        int rt = write(m_tickleFds[1], "T", 1);
        MINGFWQ_ASSERT(rt == 1);
        
    }

    bool IOManager::stopping(){
        uint64_t timeout;
        return stopping(timeout);
    }
    
    bool IOManager::stopping(uint64_t& timeout){
        timeout = getNextTimer();
        return timeout == ~0ull
                && m_pendingEventCount == 0
                && Scheduler::stopping();
    }

    void IOManager::idle(){
        //MINGFWQ_LOG_INFO(g_logger) << "idle begin";

        epoll_event* events = new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
            delete[] ptr;
        });

        
        while (true){
            uint64_t next_timeout = 0;
            if (stopping(next_timeout)){
                //stopping（next_timeout）还返回了下一次执行时间
                //也返回是否停止
                MINGFWQ_LOG_INFO(g_logger) << "m_name = " << getName() << " idle stopping exit"; 
                break;
            }

            int rt = 0;
            do{ 
                //epoll也是毫秒级，所以定时器的话也是用毫秒级就好了
                static const int MAX_TIMEOUT = 3000;
                if(next_timeout != ~0ull){
                    next_timeout = (int)next_timeout > MAX_TIMEOUT ?
                                        MAX_TIMEOUT : next_timeout;
                }else{
                    next_timeout = MAX_TIMEOUT;
                }

                rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);   
                //MINGFWQ_L  OG_INFO(g_logger) << "epoll_wait rt=" << rt;
                //EINTR 表示中断函数调用
                if(rt < 0 && errno == EINTR){

                }else {
                    break;
                }
            } while (true);
            
            //将满足定时器输出的都取出来schedule
            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if(!cbs.empty()){
                schedule(cbs.begin(),cbs.end());
                cbs.clear();
            }

            for(int i = 0; i < rt; ++i){
                epoll_event& event = events[i];

                //先看看他是不是我们用来唤醒协程的pipe
                if(event.data.fd == m_tickleFds[0]){
                    uint8_t dummy;
                    //用while 防止取不干净
                    while(read(m_tickleFds[0], &dummy, 1) == 1);
                    //唤醒完成了就行了，没有再做其他的操作
                    continue; 
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            //如果是错误或者中断的话
            if(event.events & (EPOLLERR | EPOLLHUP)){
                event.events |= EPOLLIN |EPOLLOUT;
            }

            int real_events = NONE;
            if(event.events & EPOLLIN){
                real_events |= READ;
            }
            if(event.events & EPOLLOUT){
                real_events |= WRITE;
            }

            //判断它是否是有事件
            if((fd_ctx->events & real_events) == NONE){
                continue;
            }
            //剩余的事件重新放回去,把刚才处理的事件去掉
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD: EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2){
                MINGFWQ_LOG_DEBUG(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << fd_ctx->fd << ", " << event.events << "):"
                    << rt2 << " (" << errno << ")(" << strerror(errno) << ")";
                continue;
            }

            //没有出错，也把事件改完了，就出触发事件
            if(real_events & READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }

        }

        //处理完一个事件之后，就把控制权让出来
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        //返回到scheder主协程 
        raw_ptr->swapOut(); 

        }
    }

    void IOManager::onTimerInsertedAtFront() {
        tickle();
    }

}
