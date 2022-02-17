#pragma once
#include "scheduler.h"
#include "timer.h"

namespace mingfwq{

//基于 Epoll 的IO协程调度器
class IOManager : public Scheduler,public TimerManager{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
     
     //事件类型
    enum Event{
        NONE = 0x0,     //无事件
        READ = 0x1,     //EPOLLIN
        WRITE = 0x4,    //EPOLLOUT
    };

private:
    //Socket事件上下文类 
    struct FdContext{
        typedef Mutex MutexType;

        //事件上下文类
        struct EventContext{
            Scheduler* scheduler = nullptr;     //事件执行的scheduler
            Fiber::ptr fiber;                   //事件协程
            std::function<void()> cb;           //事件的回调函数  
        };
        
        //获取事件上下文类
        EventContext& getContext(Event event);
        //重置事件上下文
        void resetContext(EventContext& ctx);
        //触发事件
        void triggerEvent(Event event);

        //事件关联的句柄
        int fd;
        //读事件的上下文
        EventContext read;
        //写事件的上下文
        EventContext write;
        //已经注册的事件
        Event events = NONE;

        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    //0 success, -1 error
    //添加事件， fd socket句柄，event 事件类型，cb回调函数
    int addEvent(int fd, Event event, std::function<void ()> cb = nullptr);
    //删除事件， 不会触发事件
    bool delEvent(int fd, Event event);
    //取消事件， 如果事件存在则强制触发事件然后执行
    bool cancelEvent(int fd, Event event);
    //取消所有事件
    bool cancelAll(int fd);
 
    static IOManager* GetThis();

protected:
    void tickle()override;
    //参数有了变化，还依赖于iomanager中的事件数m_pendingEventCount
    bool stopping()override;
    //stopping还需要返回一个时间出来
    bool stopping(uint64_t& timeout);
    /*  基于epoll实现io的，所以他会先检查epoll_wait里面有哪些事件需要唤醒
    **  如果没有事件就会陷入epoll_wait，外部在tickele()函数中tickleFds[1]写入字符
    **   ，就会让idle()检测ickeleFds[0]中可以读出数据，进而被唤醒取处理事件
    */
    void idle()override;
    //重置socket句柄上下文的容器大小
    void contextResize(size_t size);
    
    void onTimerInsertedAtFront() override;
private:
    // epoll 文件句柄
    int m_epfd = 0;
    // pipe 文件句柄  pipefd[0]读取端，pipefd[1]写入端
    int m_tickleFds[2];
    // 当前等待执行的事件数
    std::atomic<size_t> m_pendingEventCount = {0};
    
    RWMutexType m_mutex;
    // socket事件上下文容器
    std::vector<FdContext*> m_FdContexts;
};



}