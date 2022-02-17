#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>   //原子量


namespace mingfwq{

static Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id(0);
static std::atomic<uint64_t> s_fiber_count(0);

static thread_local Fiber* t_fiber = nullptr;           //当前协程
static thread_local Fiber::ptr t_threadFiber = nullptr; //main协程

static ConfigVar<uint32_t>::ptr g_fiber_statck_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }
    static void Dealloc(void* vp, size_t size){
        return free(vp);
    }
};
    
    using StackAllocator = MallocStackAllocator;

    uint64_t Fiber::GetFiberId(){
        if(t_fiber){
            return t_fiber->getId();
        }
        return 0;
    }

    Fiber::Fiber(){
        m_state = EXEC;
        SetThis(this);
        if(getcontext(&m_ctx) ){
            MINGFWQ_ASSERT2(false, "getcontext ");
        }

        ++s_fiber_count;

        MINGFWQ_LOG_DEBUG(g_logger) << "Fiber::Fiber " ;
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
        :m_id(++s_fiber_id)
        ,m_cb(cb){
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_statck_size->getValue();

        m_stack = StackAllocator::Alloc(m_stacksize);
        if(getcontext(&m_ctx)){
            MINGFWQ_ASSERT2(false, "getcontext ");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        //将新建协程和 MainFunc 关联起来
        if(!use_caller){
            makecontext(&m_ctx, &Fiber::MainFunc, 0);
        }else{
            makecontext(&m_ctx, &Fiber::CallMainFunc, 0);
        }
        
        MINGFWQ_LOG_DEBUG(g_logger) << "Fiber::Fiber id =" << m_id;
    }

    Fiber::~Fiber(){
        --s_fiber_count;
        if(m_stack){
            MINGFWQ_ASSERT(m_state == TERM 
                        || m_state == EXCEPT
                        || m_state == INIT );
            StackAllocator::Dealloc(m_stack, m_stacksize);
        } else{ 
            MINGFWQ_ASSERT(!m_cb);
            MINGFWQ_LOG_DEBUG(g_logger) << "m_state = " << m_state
                                        << " fiber_id = " << m_id;
            MINGFWQ_ASSERT(m_state == EXEC);

            Fiber* cur = t_fiber;
            if(cur == this){
                SetThis(nullptr);
            }
        }
       
        MINGFWQ_LOG_DEBUG(g_logger) << "Fiber::~Fiber id =" << m_id;
    }

    void Fiber::reset(std::function<void()> cb){
        MINGFWQ_ASSERT(m_stack);
        MINGFWQ_ASSERT(m_state ==  TERM||m_state == INIT || m_state == EXCEPT );
        
        m_cb = cb;
        if(getcontext(&m_ctx)){
            MINGFWQ_ASSERT2(false, "getcontext ");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        //将新建协程和 MainFunc 关联起来
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }
    void Fiber::swapIn(){
        SetThis(this);
        MINGFWQ_ASSERT(m_state != EXEC);
        //t_threadFiber 该值还未设置
        m_state = EXEC;
        if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx )){
            MINGFWQ_ASSERT2(false, "swapcontext");
        }

    }

    void Fiber::swapOut(){
        SetThis(Scheduler::GetMainFiber());
        if(swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx) )){
            MINGFWQ_ASSERT2(false, "swapcontext");
        }
        
    }

    void Fiber::call(){
        SetThis(this);
        m_state = EXEC;
        MINGFWQ_LOG_INFO(g_logger) << "fiber_Id = " << getId();
        if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)){
            MINGFWQ_ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::back(){
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx, &(t_threadFiber->m_ctx) )){
            MINGFWQ_ASSERT2(false, "swapcontext");
        }
        
    }

    void Fiber::SetThis(Fiber* f){
        t_fiber = f;
    }
    typename Fiber::ptr Fiber::GetThis(){
        if(t_fiber){
            return t_fiber->shared_from_this(); //给出自己的智能指针
        }
        Fiber::ptr main_fiber(new Fiber);
        MINGFWQ_ASSERT(main_fiber.get() == t_fiber);
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }
    void Fiber::YieldToReady(){
        Fiber::ptr cur = GetThis();
        cur->m_state = READY;
        cur->swapOut();
    }
    void Fiber::YieldToHold(){
        Fiber::ptr cur = GetThis();
        cur->m_state = HOLD;
        cur->swapOut();
    }
    uint64_t Fiber::TotalFibers(){
        return s_fiber_count;
    }

    void Fiber::MainFunc(){
        Fiber::ptr cur = GetThis();
        MINGFWQ_ASSERT(cur);
        try{
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }catch(std::exception& ex ){
            cur->m_state = EXCEPT;
            MINGFWQ_LOG_DEBUG(g_logger) << "Fiber Except:" << ex.what()
                    << std::endl
                    << " fiber_id = " << cur->getId()
                    << std::endl
                    << mingfwq::BacktraceToString();
        }catch(...){
            cur->m_state = EXCEPT;
            MINGFWQ_LOG_DEBUG(g_logger) << "Fiber Except"
                    << std::endl
                    << " fiber_id = " << cur->getId()
                    << std::endl
                    << mingfwq::BacktraceToString();
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();
        
    }

    void Fiber::CallMainFunc(){
        Fiber::ptr cur = GetThis();
        MINGFWQ_ASSERT(cur);
        try{
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }catch(std::exception& ex ){
            cur->m_state = EXCEPT;
            MINGFWQ_LOG_DEBUG(g_logger) << "Fiber Except:" << ex.what()
                    << std::endl
                    << " fiber_id = " << cur->getId()
                    << std::endl
                    << mingfwq::BacktraceToString();
        }catch(...){
            cur->m_state = EXCEPT;
            MINGFWQ_LOG_DEBUG(g_logger) << "Fiber Except"
                    << std::endl
                    << " fiber_id = " << cur->getId()
                    << std::endl
                    << mingfwq::BacktraceToString();
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->back();
    }
    
}

