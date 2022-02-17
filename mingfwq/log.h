#pragma once

#include <iostream>
#include <string>
#include <string.h>
#include <stdint.h>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "util.h"
#include <time.h>
#include "singleton.h"
#include <stdarg.h>
#include "thread.h"

#define MINGFWQ_LOG_LEVEL(logger,level) \
	if(logger->getLevel() <= level) \
	mingfwq::LogEventWrap(mingfwq::LogEvent::ptr(new mingfwq::LogEvent(    \
		logger,level,__FILE__,__LINE__,0,mingfwq::GetThreadId(), \
		mingfwq::GetFiberId(),time(0),mingfwq::Thread::GetName()))).getSS()

#define MINGFWQ_LOG_LIMIT(logger,level,fun) \
	if(logger->getLevel() <= level) \
	mingfwq::LogEventWrap(mingfwq::LogEvent::ptr(new mingfwq::LogEvent(    \
		logger,level,__FILE__,__LINE__,0,mingfwq::GetThreadId(), \
		mingfwq::GetFiberId(),time(0),mingfwq::Thread::GetName(),fun))).getSS()

#define MINGFWQ_LOG_DEBUG(logger) MINGFWQ_LOG_LEVEL(logger,mingfwq::LogLevel::DEBUG)
#define MINGFWQ_LOG_INFO(logger) MINGFWQ_LOG_LEVEL(logger,mingfwq::LogLevel::INFO)
#define MINGFWQ_LOG_WARN(logger) MINGFWQ_LOG_LEVEL(logger,mingfwq::LogLevel::WARN)
#define MINGFWQ_LOG_ERROR(logger) MINGFWQ_LOG_LEVEL(logger,mingfwq::LogLevel::ERROR)
#define MINGFWQ_LOG_FATAL(logger) MINGFWQ_LOG_LEVEL(logger,mingfwq::LogLevel::FATAL)

#define MINGFWQ_LOG_FMT_LEVEL(logger,level,fmt,...) \
	if(logger->getLevel() <= level) \
	mingfwq::LogEventWrap(mingfwq::LogEvent::ptr(new mingfwq::LogEvent(logger, level,__FILE__,__LINE__,0, \
	mingfwq::GetThreadId(),mingfwq::GetFiberId(),time(0),mingfwq::Thread::GetName()))).getEvent()->format(fmt,__VA_ARGS__);

#define MINGFWQ_LOG_FMT_DEBUG(logger,fmt,...) MINGFWQ_LOG_FMT_LEVEL(logger,mingfwq::LogLevel::DEBUG,fmt,__VA_ARGS__)
#define MINGFWQ_LOG_FMT_INFO(logger,fmt,...) MINGFWQ_LOG_FMT_LEVEL(logger,mingfwq::LogLevel::INFO,fmt,__VA_ARGS__)
#define MINGFWQ_LOG_FMT_WARN(logger,fmt,...) MINGFWQ_LOG_FMT_LEVEL(logger,mingfwq::LogLevel::WARN,fmt,__VA_ARGS__)
#define MINGFWQ_LOG_FMT_ERROR(logger,fmt,...) MINGFWQ_LOG_FMT_LEVEL(logger,mingfwq::LogLevel::ERROR,fmt,__VA_ARGS__)
#define MINGFWQ_LOG_FMT_FATAL(logger,fmt,...) MINGFWQ_LOG_FMT_LEVEL(logger,mingfwq::LogLevel::FATAL,fmt,__VA_ARGS__)


#define MINGFWQ_LOG_ROOT() mingfwq::LoggerMgr::GetInstance()->getRoot()
#define MINGFWQ_LOG_NAME(name) mingfwq::LoggerMgr::GetInstance()->getLogger(name)


namespace mingfwq
{
class Logger;
class LoggerManager;

//日志等级
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,
	DEBUG = 1,
	INFO = 2,
	WARN = 3,
	ERROR = 4,
	FATAL = 5
    };

    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};

//日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level ,const char* file ,int32_t m_line,uint32_t elapse, uint32_t thread_id
            ,uint32_t fiber_id, uint64_t time, const std::string& thread_name);
    LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level ,const char* file ,int32_t m_line,uint32_t elapse, uint32_t thread_id
            ,uint32_t fiber_id, uint64_t time, const std::string& thread_name, std::function<bool()> fun);
    const char* getFile() const {return m_file;}
    int32_t getLine() const {return m_line;}
    uint64_t getTime() const {return m_time;}
    uint32_t getThreadId() const {return m_threadId;}
    uint32_t getFiberId() const {return m_fiberId;}
    uint32_t getElapse() const {return m_elapse;}
    const std::string& getThreadName() const {return m_threadName;}
    //返回日志内容
    std::string getContect() const {return m_ss.str();}
    std::shared_ptr<Logger> getLogger() const {return m_logger;}
    LogLevel::Level getLevel()const {return m_level;}
    std::stringstream& getSS(){return m_ss;}

    bool getisBan()const{return m_isBan;}
    void setisBan(){
        if(!m_fun()){
            m_isBan = false;
        }else{
            m_isBan = true;
        }
    }
    std::stringstream& getLimitSS();


    void format(const char* fmt,...);
    void format(const char* fmt,va_list al);

private:
    const char* m_file = nullptr;   //文件名
    int32_t m_line = 0;		    //行号 
    uint32_t m_elapse = 0;	    //从程序开始到现在的毫秒
    uint32_t m_threadId = 0;	    //线程ID
    uint32_t m_fiberId = 0;	    //协程ID
    uint64_t m_time;		    //时间戳
    std::string m_threadName;   //线程名
    std::stringstream m_ss;	    //日志内容流
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;

    bool m_isBan;   //是否禁止输出
    std::function<bool()> m_fun;
    std::stringstream m_aa;
};

class LogEventWrap{
public:
	LogEventWrap(LogEvent::ptr e);
	~LogEventWrap();
	LogEvent::ptr getEvent()const {return m_event;}
	std::stringstream& getSS();
private:
	LogEvent::ptr m_event;
};


//日志格式器
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    //按照格式输出内容
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event);

public:
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){}
        virtual void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event ) = 0;
};

    void init();
    bool isError()const {return m_error;};
    const std::string getPattern()const { return m_pattern;}
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};


//日志输出地
class LogAppender{
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;
    virtual ~LogAppender(){}
    
    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter ();

    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;
    LogLevel::Level getLevel()const {return m_level;}
    void setLevel(LogLevel::Level val ) { m_level = val;}
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    //是否有自己的日志格式器
    bool m_hasFormatter = false;
    MutexType m_mutex;
    LogFormatter::ptr m_formatter;
};

//日志器
class Logger : public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    Logger(const std::string& name = "root");

    void log(LogLevel::Level level,LogEvent::ptr event);
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppender();
    LogLevel::Level getLevel()const {return m_level;}
    void setLevel(LogLevel::Level val) {m_level = val; }

    const std::string& getName() const {return m_name;}

    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);
    LogFormatter::ptr getFormatter();

    std::string toYamlString();

    /*
    typedef bool (*Userfunc)(void*);
    void addUserFunc(Userfunc func);
    void delUserFunc(Userfunc func);
    void clearUserFunc();
    */
    void addUserFunc(std::function<void()> func);
    void delUserFunc(std::function<void()> func);
    void clearUserFunc();

private:
    std::string m_name;		                    //日志名称
    LogLevel::Level m_level;	                //日志等级
    MutexType m_mutex;
    std::list<LogAppender::ptr> m_appenders;    //Appender集合
    LogFormatter::ptr m_formatter;

    Logger::ptr m_root;

    //std::list<Userfunc> m_func;    
};

//输出到控制台的Appender
class StdoutLogAppender:public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;
private:
    std::string toYamlString() override;

};

//定义输出到文件的Appender
class FileLogAppender:public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;
    FileLogAppender(const std::string& filename);
    std::string toYamlString() override;
    //文件重新打开，成功打开返回true
    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;
};

class LoggerManager{
public:
    typedef Spinlock MutexType;
	LoggerManager();
	Logger::ptr getLogger(const std::string& name);
	
	void init();
    Logger::ptr getRoot()const{return m_root;}
    std::string toYamlString();
private:
    MutexType m_mutex;
	//按等级分log
	std::map<std::string , Logger::ptr> m_loggers;
	//主log文件 (全部)
	Logger::ptr m_root;
};
	//日志管理类单例模式
	typedef mingfwq::Singleton<LoggerManager> LoggerMgr;

}



