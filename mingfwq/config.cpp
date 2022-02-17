#include "config.h"


namespace mingfwq{



ConfigVarBase::ptr Config::LookupBase(const std::string& name){
    RWMutexType::ReadLock lock(GetMutex());
    auto it =GetDatas().find(name);
    return it ==GetDatas().end() ? nullptr : it->second;     
}

//"A,B",10
//A:
//  B: 10
//  C: str
static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string , const YAML::Node>>& output){
        if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
            != std::string::npos){
                MINGFWQ_LOG_ERROR(MINGFWQ_LOG_ROOT() )<<"Config invalid name: "<< prefix <<" : "<<node;
                return;
            }
            output.push_back(std::make_pair(prefix,node));
            //如果是分层的比如是有两个map
            if(node.IsMap()){
                for(auto it = node.begin();
                        it != node.end(); ++it){
                    ListAllMember(prefix.empty() ? it->first.Scalar()
                                : prefix + "." + it->first.Scalar(), it->second,output);
                }
            }
}

void Config::LoadFromYaml(const YAML::Node& root){
    std::list<std::pair<std::string,const YAML::Node>> all_nodes;
    ListAllMember("",root,all_nodes);
    
    for(auto& i : all_nodes){
        std::string key = i.first;
        if(key.empty()){
            continue;
        }
        //将key中的大写字母转化成小写字母
        std::transform(key.begin(),key.end(),key.begin(),::tolower);
        
        ConfigVarBase::ptr var = LookupBase(key);
        if(var){
            //是不是string，简单类型
            if(i.second.IsScalar()){
                var->fromString(i.second.Scalar());
            }else{          
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

void Config::Visit(std::function<void (ConfigVarBase::ptr)> cb){
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin();
            it != m.end(); ++it){
                cb(it->second);
            }
}

}
