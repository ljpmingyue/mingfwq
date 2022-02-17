#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace mingfwq{

    static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");
    static thread_local Scheduler* t_scheduler = nullptr;   //局部的协程调度器
    static thread_local Fiber* t_fiber = nullptr;           //局部的主协程(调度协程)

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
            :m_name(name){
        MINGFWQ_ASSERT(threads > 0);

        if(use_caller){
            /*
                在一个线程里面声名一个调度器，再把当前线程放到调度器里面，
                它的主协程不再是线程的主协程，而是执行run方法的协程
            */
            mingfwq::Fiber::GetThis();
            --threads;
            MINGFWQ_ASSERT(GetThis() == nullptr);
            t_scheduler = this;
            //MINGFWQ_LOG_INFO(g_logger) << "调度协程 after it";
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run , this), 0, true));
            mingfwq::Thread::SetName(m_name);

            t_fiber = m_rootFiber.get();
            m_rootThread = mingfwq::GetThreadId();
            m_threadIds.push_back(m_rootThread); 
        } else {
            //没有线程id
            m_rootThread = -1;
        }

        m_threadCount = threads;
    }

    Scheduler::~Scheduler(){
        MINGFWQ_ASSERT(m_stopping);
        if(GetThis() == this){
            t_scheduler = nullptr;
        }
    }

    Scheduler* Scheduler::GetThis(){
        return t_scheduler;
    }

    Fiber* Scheduler::GetMainFiber(){
        return t_fiber;
    }
    
    void Scheduler::start(){
        //MINGFWQ_LOG_INFO(g_logger) << " Scheduler::start() ";
        MutexType::Lock lock(m_mutex);
        if(!m_stopping){    //如果m_stopping为true则还没启动
            return; 
        }
        m_stopping = false;
        MINGFWQ_ASSERT(m_threads.empty());

        m_threads.resize(m_threadCount);
        for(size_t i = 0; i < m_threadCount; ++ i){
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                                    ,m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();

        /*
        if(m_rootFiber){
            //m_rootFiber->swapIn(); 在中间用到了调度协程（主协程）切换掉变成自己的协程
            m_rootFiber->call();
            MINGFWQ_LOG_INFO(g_logger) << " Scheduler::run m_rootFiber->call() out ";
        }
        */

    }
    void Scheduler::stop(){
        MINGFWQ_LOG_INFO(g_logger) << " Scheduler::stop() ";
        m_autoStop = true;
        if(m_rootFiber && m_threadCount == 0
                && (m_rootFiber->getState() == Fiber::State::TERM
                || m_rootFiber->getState() == Fiber::INIT)){
            MINGFWQ_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;
            if(stopping() ){
                return;         //用了use_call只有一个线程从这里返回
            }
        } 

        //bool exit_on_this_fiber = false;
        /*
            调度器创建有两种，一种是用了use_call: 在创建了Scheduler的线程里去执行stop
            另一种是不用：在非它的线程内执行stop就行了
        */
        if(m_rootThread != -1){
            //---------------------------------------
            MINGFWQ_ASSERT(GetThis() == this);
        } else {
            MINGFWQ_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for(size_t i = 0; i < m_threadCount; ++i){
            tickle();
        }

        if(m_rootFiber){
            tickle();
        }

        //它为true则说明它用的是原来的线程
        if(m_rootFiber){    
            /*
            //为了让主协程等待其他线程的协程返回
            while (!stopping()){
                
                //如果发现m_rootFiber已经结束了就重新建立一个
                if(m_rootFiber->getState() == Fiber::TERM
                        || m_rootFiber->getState() == Fiber::EXCEPT ){
                    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run , this), 0, true));
                    MINGFWQ_LOG_INFO (g_logger) << "root fiber is term, reset";
                    t_fiber = m_rootFiber.get();
                }
                m_rootFiber->call();
            }
            */
           if(!stopping()){
                   m_rootFiber->call();
               }
        }
        
        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        for(auto i: thrs){
            i->join(); 
        }
        
        //if(exit_on_this_fiber){}

    }

    void Scheduler::run(){
        MINGFWQ_LOG_INFO(g_logger) << " Scheduler::run() "; 
        set_hook_enable(true);
        setThis();
        if(mingfwq::GetThreadId() != m_rootThread){
            t_fiber = Fiber::GetThis().get();
        }
        //---------------------------------
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while (true)
        {
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    if(it->thread != -1 && it->thread != mingfwq::GetThreadId()){
                        ++it;
                        tickle_me = true;   //这个任务虽然本线程不需要处理，
                                            //但是也是需要发出信号通知下一个可能可以处理该任务的线程来
                        continue;
                    }

                    MINGFWQ_ASSERT(it->fiber || it->cb);    //起码需要满足一种
                    if(it->fiber && it->fiber->getState() == Fiber::State::EXEC ){
                        ++it;
                        continue; 
                    }

                    ft = *it;           //可以处理的就拿出来
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
                tickle_me |=it != m_fibers.end();
            }

            if(tickle_me){
                tickle();
            }

            //开始执行拿出来的协程
            if(ft.fiber && ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT){
                
                ft.fiber->swapIn();
                --m_activeThreadCount;

                //执行结束返回后
                //如果fiber是被yieldToReady出来的就重新放到调度协程中
                if(ft.fiber->getState() == Fiber::READY){
                    schedule(ft.fiber);
                }else if(ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT ){
                    ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();

            }else if( ft.cb ){
                if(cb_fiber){
                    //这里的reset使用的是fiber里面的reset
                    cb_fiber->reset(ft.cb);
                }else {
                    //如果cb_fiber一开始没有值就用指针里的reset
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                cb_fiber->swapIn();
                --m_activeThreadCount;
                if(cb_fiber->getState() == Fiber::READY){
                    schedule(cb_fiber);
                    cb_fiber.reset();   //智能指针的reset
                }else if(cb_fiber->getState() == Fiber::TERM
                        || cb_fiber->getState() == Fiber::EXCEPT ){
                    cb_fiber->reset(nullptr);
                }else {
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();   //释放智能指针
                }
            }else { //没任务做的时候
                if(is_active){
                    --m_activeThreadCount;
                    continue;
                }

                if(idle_fiber->getState() == Fiber::TERM ){
                    MINGFWQ_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                    //continue;

                }
                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if(idle_fiber->getState() != Fiber::TERM 
                        && idle_fiber->getState() != Fiber::EXCEPT ){
                    idle_fiber->m_state = Fiber::State::HOLD;
                }
                
            }
        } 
        
    }

    void Scheduler::setThis(){
        t_scheduler = this;
    }

    void Scheduler::idle(){
        MINGFWQ_LOG_INFO(g_logger) << "idle";
        while (!stopping()){
            mingfwq::Fiber::YieldToHold();
        }
        
    }

    void Scheduler::tickle(){
        MINGFWQ_LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping(){
        MutexType::Lock lock(m_mutex);
        //消息队列里没有要执行的，也没有正在运行的线程
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }


}