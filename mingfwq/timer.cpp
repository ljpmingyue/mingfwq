#include "timer.h"
#include "util.h"
#include "log.h"

namespace mingfwq{
    static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_NAME("system");
    Timer::Timer(uint64_t ms, std::function<void()> cb,
                bool recurring, TimerManager* manager)
            :m_recurring(recurring)
            ,m_ms(ms)
            ,m_cb(cb)
            ,m_manager(manager){
        m_next = mingfwq::GetCurrentMS() + m_ms;
    }
    Timer::Timer(uint64_t next)
            :m_next(next){

    }

    bool Timer::cancel(){
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(m_cb){
            
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }
    
    bool Timer::refresh(){
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb){
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }
        //为什么要先删掉再修改添加：
        //set<Timer::ptr,Timer::Comparator> m_timers的比较方法是自己定的，修改之后就位置会发生变化
        m_manager->m_timers.erase(it);
        m_next = mingfwq::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
         
    }

    bool Timer::reset(uint64_t ms, bool from_now){
        //时间没变返回false
        if(ms == m_ms && !from_now){
            return false;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb){
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }
        //为什么要先删掉再修改添加：
        //set<Timer::ptr,Timer::Comparator> m_timers的比较方法是自己定的，修改之后就位置会发生变化
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if(from_now){
            start = mingfwq::GetCurrentMS();
        }else {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }


    bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const{
        if(!lhs & !rhs){
            return false;
        }
        if(!lhs){
            return true;
        }
        if(!rhs){
            return false;
        }
        if(lhs->m_next < rhs->m_next){
            return true;
        }
        if(lhs->m_next > rhs->m_next){
            return false;
        }
        return lhs.get() < rhs.get();

    }


    TimerManager::TimerManager(){

    }

    TimerManager::~TimerManager(){

    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring ){
        Timer::ptr timer(new Timer(ms,cb,recurring,this) );
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer,lock);
        return timer;
    }

    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock){
        //返回它的位置
        auto it = m_timers.insert(val).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled; 
        if(at_front){
            m_tickled = true;
        }
        lock.unlock();
        if(at_front){
            onTimerInsertedAtFront();
        }
    }


    static void OnTime(std::weak_ptr<void> weak_cond, std::function<void()> cb){
        //lock()，返回weak_ptr的智能指针，没释放就拿到指针，释放了就是空的
        std::shared_ptr<void> tmp = weak_cond.lock();
        if(tmp){
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                        std::weak_ptr<void> weak_cond,
                        bool recurring ){
        return addTimer(ms, std::bind(&OnTime, weak_cond, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer(){
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if(m_timers.empty() ){
            //0取反，最大的数
            return ~0ull;
        }
        const Timer::ptr& next = *m_timers.begin();
        uint64_t now_ms = mingfwq::GetCurrentMS();
        if(now_ms >= next->m_next){
            return 0;
        }else {
            return next->m_next - now_ms;
        }
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs){
        uint64_t now_ms = mingfwq::GetCurrentMS();
        //存放已经超时或到时间的定时器
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if(m_timers.empty()){
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        Timer::ptr now_timer(new Timer(now_ms));
        
        //二分查找的方法——lower_bound
        auto it = m_timers.lower_bound(now_timer);

        while (it != m_timers.end() && (*it)->m_next == now_ms){
            ++it;
        }
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);

        cbs.reserve(expired.size());
        for(auto& timer : expired){
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring){
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }else {
                //防止回调函数中有用到指针指针，没置空引用不会减一
                timer->m_cb = nullptr;
            }
        }
    }

    bool TimerManager::hasTimer(){
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

}
