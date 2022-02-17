#pragma once

#include "thread.h"

#include <set>
#include <memory.h>
#include <vector>

namespace mingfwq{

class TimerManager;

class Timer:public std::enable_shared_from_this<Timer>{
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    
    //取消定时器
    bool cancel();
    //刷新定时器的执行时间
    bool refresh();
    //重新设定定时器的时间，ms执行间隔，from_now是否从当前时间开始算
    bool reset(uint64_t ms, bool from_now);

private:
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);
    Timer(uint64_t next);
private:
    //是否循环定时器
    bool m_recurring = false;
    //执行周期
    uint64_t m_ms = 0;
    //精确的执行时间
    uint64_t m_next = 0;
    std::function<void()> m_cb;
    TimerManager* m_manager = nullptr;
private:
    struct Comparator{
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs)const;
    };

};

class TimerManager{
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    //这个功能给IOManager继承获得的
    virtual ~TimerManager();
    /*  添加定时器
    **  ms 定时器执行的时间间隔
    **  cb 定时器回调函数
    **  recurring 是否循环定时器
    */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring = false);
    /*  条件定时器    当他要触发的时候且条件还存在的时候
        用智能指针来做条件，指针消失了就没必要实现了
        场景：触发的时候依赖条件已经发生变化了，我需要的东西已经不存在了
        std::weak_ptr<void> weak_cond 条件
    */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                        std::weak_ptr<void> weak_cond,
                        bool recurring = false);
    //到最近一个定时器执行的时间间隔
    uint64_t getNextTimer();
    /*  获取需要执行的定时器的回调函数列表
    **  返回在m_timers中到时间的或者超时的
    */
    void listExpiredCb(std::vector<std::function<void()> >& cbs);
    //是否还有定时器
    bool hasTimer();
protected:
    /*  当有新的定时器插入到定时器的首部,执行该函数
    **  因为此时schedule去epoll_wait的时候，提醒有时间更短的任务进来了
    **  让schedule把epoll_wait需要把时间调整一下
    */
    virtual void onTimerInsertedAtFront() = 0;
    //将定时器添加到管理器中,判断是不是添加到m_timers的最前面，
    //不需要这么判断的就可以用insert
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);

private:
    //
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;
    std::set<Timer::ptr,Timer::Comparator> m_timers;
    //是否触发过onTimerInsertedAtFront,
    //将at_front限制加多，可以避免多次执行onTimerInsertedAtFront函数
    bool m_tickled = false;
    //上一次的执行时间
    uint64_t m_previouseTime = 0;
};

}