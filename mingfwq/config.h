#pragma once 

#include <iostream>
#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "log.h"
#include "thread.h"
#include "config.h"
namespace mingfwq{
//配置变量的基类
class ConfigVarBase{
public:
	typedef std::shared_ptr<ConfigVarBase> ptr;
	ConfigVarBase(const std::string& name,const std::string& description ="" )
		:m_name(name),
		m_description(description){
	}
	virtual ~ConfigVarBase(){}

	const std::string& getName()const{return m_name;}
	const std::string& getDescription()const{return m_description;}
	
	//转化成字符串
	virtual std::string toString()=0;
	//从字符串初始化值
	virtual bool fromString(const std::string val) = 0;

	virtual std::string getTypeName()const = 0;

protected:
	std::string m_name;
	std::string m_description;
};

//F from_type , T to_type
template<class F,class T>
class LexicalCast{
public:
	T operator()(const F& v){
		return boost::lexical_cast<T>(v);
	}
};


//YAML中的string 转化成 vecter<T>
template<class T>
class LexicalCast<std::string, std::vector<T> >{
public:
	std::vector<T> operator() (const std::string& v){

		typename std::vector<T> vec;
		YAML::Node node = YAML::Load(v);
		std::stringstream ss;
		//把他当一个数组，如果不是数组  在ConfigVar::fromString函数处抛出异常，提示转化失败，会catch这个异常
		for(size_t i = 0; i < node.size(); ++i){
			ss.str("");
			ss << node[i];
			vec.push_back(LexicalCast<std::string,T>()(ss.str()));
		}
		return vec;
	}
};
//YAML中的vector 转化成 string
template<class T>
class LexicalCast< std::vector<T>, std::string >{
public:
	std::string operator() (const std::vector<T>& v){
		YAML::Node node;
		for(auto& i : v){
			//Load是将输入字符串作为单个YAML文档加载
			node.push_back(YAML::Load(LexicalCast<T,std::string>()(i) ));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};



//YAML中的string 转化成 list
template<class T>
class LexicalCast<std::string, std::list<T> >{
public:
	std::list<T> operator() (const std::string& v){

		typename std::list<T> vec;
		YAML::Node node = YAML::Load(v);
		std::stringstream ss;
		//把他当一个数组，如果不是数组  在ConfigVar::fromString函数处抛出异常，提示转化失败，会catch这个异常
		for(size_t i = 0; i < node.size(); ++i){
			ss.str("");
			ss << node[i];
			vec.push_back(LexicalCast<std::string,T>()(ss.str()));
		}
		return vec;
	}
};
//YAML中的list 转化成 string
template<class T>
class LexicalCast< std::list<T>, std::string >{
public:
	std::string operator() (const std::list<T>& v){
		YAML::Node node;
		for(auto& i : v){
			//Load是将输入字符串作为单个YAML文档加载
			node.push_back(YAML::Load(LexicalCast<T,std::string>()(i) ));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};


//YAML中的string 转化成 set
template<class T>
class LexicalCast<std::string, std::set<T> >{
public:
	std::set<T> operator() (const std::string& v){

		typename std::set<T> vec;
		YAML::Node node = YAML::Load(v);
		std::stringstream ss;
		//把他当一个数组，如果不是数组  在ConfigVar::fromString函数处抛出异常，提示转化失败，会catch这个异常
		for(size_t i = 0; i < node.size(); ++i){
			ss.str("");
			ss << node[i];
			vec.insert(LexicalCast<std::string,T>()(ss.str()));
		}
		return vec;
	}
};
//YAML中的set 转化成 string
template<class T>
class LexicalCast< std::set<T>, std::string >{
public:
	std::string operator() (const std::set<T>& v){
		YAML::Node node;
		for(auto& i : v){
			//Load是将输入字符串作为单个YAML文档加载
			node.push_back(YAML::Load(LexicalCast<T,std::string>()(i) ));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};


//YAML中的string 转化成 unorder_set
template<class T>
class LexicalCast<std::string, std::unordered_set<T> >{
public:
	std::unordered_set<T> operator() (const std::string& v){

		typename std::unordered_set<T> vec;
		YAML::Node node = YAML::Load(v);
		std::stringstream ss;
		//把他当一个数组，如果不是数组  在ConfigVar::fromString函数处抛出异常，提示转化失败，会catch这个异常
		for(size_t i = 0; i < node.size(); ++i){
			ss.str("");
			ss << node[i];
			vec.insert(LexicalCast<std::string,T>()(ss.str()));
		}
		return vec;
	}
};
//YAML中的unordered_set 转化成 string
template<class T>
class LexicalCast< std::unordered_set<T>, std::string >{
public:
	std::string operator() (const std::unordered_set<T>& v){
		YAML::Node node;
		for(auto& i : v){
			//Load是将输入字符串作为单个YAML文档加载
			node.push_back(YAML::Load(LexicalCast<T,std::string>()(i) ));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

//YAML中的string 转化成 map<std::string, T>
template<class T>
class LexicalCast<std::string, std::map<std::string, T> >{
public:
	std::map<std::string, T> operator() (const std::string& v){
		YAML::Node node = YAML::Load(v);
		typename std::map<std::string, T> vec;
		std::stringstream ss;
		//把他当一个数组，如果不是数组  在ConfigVar::fromString函数处抛出异常，提示转化失败，会catch这个异常
		for(auto it = node.begin();
				it != node.end(); ++it ){
			ss.str("");
			ss << it->second;
			vec.insert(std::make_pair(it->first.Scalar(),
						LexicalCast<std::string, T>() (ss.str() )));
		}
		return vec;
	}
};
//YAML中的 map<std::string, T> 转化成 string
template<class T>
class LexicalCast< std::map<std::string, T>, std::string >{
public:
	std::string operator() (const std::map<std::string, T>& v){
		YAML::Node node;
		for(auto& i : v){
			//Load是将输入字符串作为单个YAML文档加载
			node[i.first] = YAML::Load(LexicalCast<T,std::string>()(i.second));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

//YAML中的string 转化成 unordered_map<std::string, T>
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T> >{
public:
	std::unordered_map<std::string, T> operator() (const std::string& v){
		YAML::Node node = YAML::Load(v);
		typename std::unordered_map<std::string, T> vec;
		std::stringstream ss;
		//把他当一个数组，如果不是数组  在ConfigVar::fromString函数处抛出异常，提示转化失败，会catch这个异常
		for(auto it = node.begin();
				it != node.end(); ++it ){
			ss.str("");
			ss << it->second;
			vec.insert(std::make_pair(it->first.Scalar(),
						LexicalCast<std::string, T>() (ss.str() )));
		}
		return vec;
	}
};
//YAML中的 map<std::string, T> 转化成 string
template<class T>
class LexicalCast< std::unordered_map<std::string, T>, std::string >{
public:
	std::string operator() (const std::unordered_map<std::string, T>& v){
		YAML::Node node;
		for(auto& i : v){
			//Load是将输入字符串作为单个YAML文档加载
			node[i.first] = YAML::Load(LexicalCast<T,std::string>()(i.second));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

//配置参数模板子类,保存对应类型的参数值
//FromStr: T operator()(const std::string&)
//ToStr:   std::string operator()(const T&)
template<class T , class FromStr = LexicalCast<std::string,T>
				 , class ToStr = LexicalCast<T , std::string> >
class ConfigVar:public ConfigVarBase{
public:
	typedef RWMutex RWMutexType;
	typedef std::shared_ptr<ConfigVar> ptr;
	typedef std::function<void (const T& old_value,const T& new_value)> on_change_cb;

	ConfigVar(const std::string& name,
			const T& default_value,
			const std::string& description = "")
		:ConfigVarBase(name,description),
		m_val(default_value){

	}
	std::string toString()override{
		try{
			RWMutexType::ReadLock lock(m_mutex);
			return ToStr()(m_val);
			//return boost::lexical_cast<std::string> (m_val);
		}catch(std::exception& e){
			MINGFWQ_LOG_ERROR(MINGFWQ_LOG_ROOT())<<"ConfigVar::toString exception"
			<<e.what()<<"convert: "<<typeid(m_val).name()<<"toString";
		}
		return "";
	}
	bool fromString(const std::string val) override {
		try{
			setValue(FromStr()(val));
			//m_val=boost::lexical_cast<T>(val);
		}catch(std::exception& e){
			MINGFWQ_LOG_ERROR(MINGFWQ_LOG_ROOT())<<"ConfigVar::toString exception"
			<<e.what()<<"convert: string to"<<typeid(m_val).name()
			<< " - " << val;
		}

		return false;
	}

	const T getValue(){
		RWMutexType::ReadLock lock(m_mutex);
		return m_val;
	}
	void setValue(const T& v){
		{//局部域  出了这个读锁失效
			RWMutexType::ReadLock lock(m_mutex);
			if(v == m_val){
				return;
			}
			for(auto& i : m_cbs){
				i.second(m_val, v);
			}
		}
		RWMutexType::WriteLock lock(m_mutex);
		m_val = v;
	}
	std::string getTypeName()const override {return typeid(T).name();}

	uint64_t addListener( on_change_cb cb){
		static uint64_t s_fun_id = 0;
		RWMutexType::WriteLock lock(m_mutex);
		++s_fun_id;
		m_cbs[s_fun_id] = cb;
		return s_fun_id;
	}

	void delListener(uint64_t key){
		RWMutexType::WriteLock lock(m_mutex);
		m_cbs.erase(key);
	}

	on_change_cb getListener(uint64_t key){
		RWMutexType::ReadLock lock(m_mutex);
		auto it = m_cbs.find(key);
		return it == m_cbs.end()? nullptr : it->second;
	}

	void clearListener(){
		RWMutexType::WriteLock lock(m_mutex);
		m_cbs.clear();
	}
private:
	RWMutexType m_mutex;

	T m_val;
	//变更回调函数组,uint64_t key,要求唯一，一般可以用hash
	std::map<uint64_t ,on_change_cb> m_cbs;
};

//configvar的管理类  提供方法访问创建configvar
class Config{
public:
	typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
	typedef RWMutex RWMutexType;
	//没有就创建，有就获取对应参数名的配置参数
	template<class T>
	static typename ConfigVar<T>::ptr Lookup(const std::string& name,
				const T& default_value, const std::string& description= "" ){
		RWMutexType::WriteLock lock(GetMutex());
		auto it = GetDatas().find(name);
		if(it != GetDatas().end()){
			auto tmp = std::dynamic_pointer_cast<ConfigVar<T>> (it->second);
		if(tmp){
			MINGFWQ_LOG_INFO( MINGFWQ_LOG_ROOT() ) << "Lookup name = "<<name<<" exists";
			return tmp;	
		}else{
			MINGFWQ_LOG_ERROR( MINGFWQ_LOG_ROOT() ) << "Lookup name = "<<name<<" exists but type not "
			<< typeid(T).name() << " real_type=" << it->second->getTypeName() <<" "
			<< it->second->toString();
			return nullptr;	
		}

		}
		
		//默认name只能在abcdefghijklmnopqrstuvwxyz._012345678中出现的字符串
		//判断name有没有和字符串里没有的，都有返回string::npos 找到不一样的输出位置
		if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")
			!= std::string::npos){
				MINGFWQ_LOG_ERROR( MINGFWQ_LOG_ROOT() )<<"Lookup name invalid "<<name;
				//抛出错误
				throw std::invalid_argument(name);
		}

		typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));
		GetDatas()[name] = v;
		return v;
	}
	
	template<class T>
	static typename ConfigVar<T>::ptr Lookup(const std::string& name){
		RWMutexType::ReadLock lock(GetMutex());
		auto it = GetDatas().find(name);
		if(it == GetDatas().end()){
			return nullptr;
		}
		return std::dynamic_pointer_cast<ConfigVar<T>> (it->second);
	}

	//使用YAML::Node初始化配置模块
	static void LoadFromYaml(const YAML::Node& root);
	//查看配置参数，返回配置参数的基类指针
	static ConfigVarBase::ptr LookupBase(const std::string& name);
	
	//遍历配置模块里的所有配置项   cb配置项回调函数
	//回调函数 ？？？？？
	static void Visit(std::function<void (ConfigVarBase::ptr)> cb);
private:
	//储存 map<std::string, ConfigVarBase::ptr> 
	static ConfigVarMap& GetDatas(){
		static ConfigVarMap s_datas;
		return s_datas;
	}
	static RWMutexType& GetMutex(){
		static RWMutexType s_mutex;
		return s_mutex;
	}
};


}
