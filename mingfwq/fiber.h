#pragma once

#include "thread.h"
#include <functional>
#include <ucontext.h>
#include <memory>


namespace mingfwq{
class scheduler;

class Fiber : public std::enable_shared_from_this<Fiber>{
friend class Scheduler;//scheduler 直接修改了m_state所以需要设置友函数
public:

    typedef std::shared_ptr<Fiber> ptr;

    enum State{
        INIT,   //初始化状态
        HOLD,   //暂停状态
        EXEC,   //执行中状态
        TERM,   //结束状态
        READY,  //可执行状态
        EXCEPT   //异常状态
    };
private:
    Fiber();    //每个线程第一个协程的构造，所以不让它delete

public:
    //cb 协程执行的函数     stacksize协议栈大小   
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();
    //获得当前状态
    State getState(){return m_state;}
    //重置协程执行函数，并设置状态
    //init, term
    void reset(std::function<void()> cb);
    //将当前协程切换成运行状态
    void swapIn();
    //将当前协程切换到后台
    void swapOut();
    //将当前线程切换到执行状态, 执行的为当前线程的主协程
    void call();
    //将当前线程切换到后台,执行的为该协程,返回到线程的主协程
    void back();

    uint64_t getId()const { return m_id; }
public:
    //设置当前线程的运行协程
    static void SetThis(Fiber* f);
    //返回当前协程  
    static typename Fiber::ptr GetThis();
    //将当前协程切换到后台，并设置成Ready状态,可执行状态
    static void YieldToReady();
    //将当前协程切换到后台，并设置成Hold状态, 暂停状态
    static void YieldToHold();
    
    
    
    //总协程数
    static uint64_t TotalFibers();
    /*
    **  协程执行函数
    **  执行完成返回到线程主协程
    */
    static void MainFunc();
    static void CallMainFunc();
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;
    //协程运行栈大小
    uint32_t m_stacksize;   
    State m_state = INIT;
    //协程的上下文
    ucontext_t m_ctx;
    //协程运行栈指针
    void* m_stack = nullptr;
    std::function<void()> m_cb;
};


}
