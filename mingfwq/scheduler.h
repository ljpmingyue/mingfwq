#pragma once 
#include "thread.h"
#include "fiber.h"

#include <memory>
#include <vector>
#include <list>

namespace mingfwq{

//协程调度器  内有线程池，支持协程在线程池里面切换
class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /*
    **      threads 线程数量
    **      是否使用当前调用线程
    **      协程调度器名称
    */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();
    //返回协程调度器名称
    const std::string& getName()const{ return m_name; }
    //返回当前协程调度器
    static Scheduler* GetThis();
    //返回当前协程调度器的调度协程
    static Fiber* GetMainFiber();
    //启动协程调度器
    void start();   
    //停止协程调度器
    void stop();
    
    /*
    **      调度协程
    **      fc 为协程或者函数cb
    **      thread 协程执行的线程id, -1标识任意线程
    */
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_ticke = false;
        {
            MutexType::Lock lock(m_mutex);
            need_ticke = scheduleNoLock(fc,thread); 
        }
        if(need_ticke){
            tickle();
        }
    }
    //批量处理, 这组连续的任务，在连续的消息队列里面，一定会按照连续的顺序进行
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end){
        bool need_ticke = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end)
            {
                need_ticke = scheduleNoLock(&*begin , -1) || need_ticke;
                ++begin;
            }
        }
        if(need_ticke){
            tickle();
        }
    }

protected:
    //通知协程调度器有任务了
    virtual void tickle();
    //协程调度函数
    void run ();
    //返回是否可以停止，让子类有清理任务的机会,判断协程调度器是否可以退出了
    virtual bool stopping();
    /*
        没任务做的时候就执行idle
        解决协程调度器又没有任务做，又不能使线程终止这个问题
        具体实现看子类
    */
    virtual void idle();
    //设置当前的协程调度器
    void setThis();
    
    bool hasIdleThreads(){return m_idleThreadCount > 0;}

private:  
    //协程调度启动(无锁)
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        //确保fc 不是协程就是执行函数
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }


private:
    struct FiberAndThread
    {
        Fiber::ptr fiber;           //协程
        std::function<void()> cb;   //协程执行函数
        int thread;                 //线程id

        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f),thread(thr){

        }
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr){
            fiber.swap(*f);
        }
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f),thread(thr){

        }
        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr){
            cb.swap(*f);
        }
        FiberAndThread()
            :thread(- 1){
        }

        void reset(){
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
    

private: 
    //Mutex
    MutexType m_mutex;
    //线程池
    std::vector<Thread::ptr> m_threads;
    //待执行的协程队列
    std::list<FiberAndThread> m_fibers;//即将要执行的和计划执行的协程
    //协程调度器名称
    std::string m_name;
    //use_call为true时，调度协程
    Fiber::ptr m_rootFiber;

protected:
    //协程调度器下的线程id数组
    std::vector<int> m_threadIds;
    //线程总数 
    size_t m_threadCount = 0;
    //工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    //空闲线程数量
    std::atomic<size_t> m_idleThreadCount = {0};
    //是否正在停止
    bool m_stopping = true;
    //是否自动停止
    bool m_autoStop = false;
    //主线程id use_call
    int m_rootThread = 0;
};



}