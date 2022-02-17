#include "thread.h"
#include "log.h" 
#include "util.h"


namespace mingfwq{

    //线程局部变量，指向当前线程
    static thread_local Thread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKONW";
    //系统的日志都打到system中
    static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");



    Semaphore::Semaphore(uint32_t count){
        if(sem_init(&m_semaphore, 0, count) ){
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore(){
        sem_destroy(&m_semaphore);
    }
     
    void Semaphore::wait(){
        //返回值非零为失败
        if(sem_wait(&m_semaphore) ){
            throw std::logic_error("sem_wait error");;
        }

    }

    void Semaphore::notify(){
        if(sem_post(&m_semaphore) ){
            throw std::logic_error("sem_post error");
        }
    }




    Thread* Thread::GetThis(){
        return t_thread;
    }
    const std::string& Thread::GetName(){
        return t_thread_name;
    }

    void Thread::SetName(const std::string& name){
        if(t_thread){
            t_thread->m_name = name;
        }
        t_thread_name = name;
        
    }

    Thread::Thread(std::function<void()> cb, const std::string& name)
        :m_cb(cb)
        ,m_name(name){

        if(name.empty()){
            m_name = "UNKNOW";
        }
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if(rt){
            MINGFWQ_LOG_DEBUG(g_logger) << "pthread_creat thread fail ,rt = " << rt
                                        << " name = " << name;
            throw std::logic_error("pthread_creat error");
        }
        //pthread_create创建的线程在Thread构造函数返回时还没开始执行
        m_semaphore.wait();
    }

    Thread::~Thread(){
        if(m_thread){
            pthread_detach(m_thread);
        }
    }

    void* Thread::run(void* arg){
        Thread* thread = (Thread*)arg;
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = mingfwq::GetThreadId();
        //给线程命名,pthread_setname_np(pthread_t , char*)
        //c_str()返回string字符的首地址
        pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());

        //当函数存在智能指针，防止它的引用会出现不释放的情况
        std::function<void()> cb;
        cb.swap(thread->m_cb);

        //这里使static不能直接使用m_semaphore
        thread->m_semaphore.notify();
        
        cb();
        return 0;
    }

    void Thread::join(){
        if(m_thread){
            int rt = pthread_join(m_thread, nullptr);
            if(rt){
                MINGFWQ_LOG_DEBUG(g_logger) << "pthread_join thread fail ,rt = " << rt
                                        << " name = " << m_name;
            throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }

   


}
