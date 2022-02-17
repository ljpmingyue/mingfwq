#pragma once 
//线程用C++11 互斥量用pc线程库

#include <thread>
#include <functional>
#include <memory> //智能指针include
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>

#include "noncopyable.h"
namespace mingfwq{

//信息量
class Semaphore : Noncopyable{
public:
    // cout 信号量值的大小
    Semaphore(uint32_t count = 0);
    ~Semaphore();
    //获得信号量
    void wait();
    //释放信号量
    void notify();

private:
    sem_t m_semaphore;
};

//局部锁的模板实现
template<class T>
struct ScopedLockImpl{
public:
    ScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.lock();
                m_locked = true;
            }
    ~ScopedLockImpl(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.lock();
        }
    }
    void unlock(){
        if(m_locked ){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    //是否已上锁
    bool m_locked;
};
//局部读锁
template<class T>
struct ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.rdlock();
                m_locked = true;
            }
    ~ReadScopedLockImpl(){
        unlock();
    }
    void rdlock(){
        if(!m_locked){
            m_mutex.rdlock();
        }
    }
    void unlock(){
        if(m_locked ){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

//局部写锁
template<class T>
struct WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.wrlock();
                m_locked = true;
            }
    ~WriteScopedLockImpl(){
        unlock();
    }

    void wrlock(){
        if(!m_locked){
            m_mutex.wrlock();
        }
    }

    void unlock(){
        if(m_locked ){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

//互斥量
class Mutex : Noncopyable{
public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex(){
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    void lock(){
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

class NullMutex: Noncopyable{
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex(){}
    ~NullMutex(){}
    void lock(){}
    void unlock(){}
private:
    pthread_mutex_t m_mutex;
};

//读写互斥量
class RWMutex : Noncopyable{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
    RWMutex(){
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex(){
        //销毁锁
        pthread_rwlock_destroy(&m_lock);
    }
    //上读锁
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }
    //上写锁
    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }
    //解锁
    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }

private:
    //读写锁
    pthread_rwlock_t m_lock;
};

class NullRWMutex : Noncopyable{
public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex(){}
    ~NullRWMutex(){}
    void rdlock(){}
    void wrlock(){}
    void unlock(){}
};

//自旋锁  空跑一段 cpu消耗高 
class Spinlock : Noncopyable{
public:
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock(){
        pthread_spin_init(&m_mutex, 0);
    }
    ~Spinlock(){
        pthread_spin_destroy(&m_mutex);
    }
    void lock(){
        pthread_spin_lock(&m_mutex);
    }
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

class CASLock : Noncopyable{
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock(){
        m_mutex.clear();
    }
    ~CASLock(){

    }
    void lock(){
        while(std::atomic_flag_test_and_set_explicit(&m_mutex,std::memory_order_acquire) );
    }
    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;
};


class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();
    pid_t getId()const {return m_id;}
    const std::string& getName() const {return m_name;}

    void join();

    //获得自己当前线程的指针
    static Thread* GetThis();
    //给日志用的getname
    static const std::string& GetName();
    //如果主线程不是自己创建的就没办法命名 用static
    static void SetName(const std::string& name);
private:
    //函数删掉
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator= (const Thread&) = delete;
    //线程执行函数
    static void* run(void* arg);
private:
    //线程id
    pid_t m_id = -1;
    //线程结构体       在pthreadtypes.h 有说明 unsigned long int类型
    pthread_t m_thread = 0; 
    //线程执行函数
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;
};

}
