#include "../mingfwq/config.h"
#include "../mingfwq/log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

mingfwq::ConfigVar<int>::ptr g_int_value_config =
	mingfwq::Config::Lookup("system.port",(int)8080,"systrem port");
/*
mingfwq::ConfigVar<float>::ptr g_int_valuex_config =
	mingfwq::Config::Lookup("system.port",(float)8080,"systrem port");
*/
mingfwq::ConfigVar<float>::ptr g_float_value_config =
	mingfwq::Config::Lookup("system.value",(float)10.2f,"systrem value");

mingfwq::ConfigVar<std::vector<int>>::ptr g_int_vector_value_config =
	mingfwq::Config::Lookup("system.int_vec",std::vector<int>{1,2},"systrem int vec");

mingfwq::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
	mingfwq::Config::Lookup("system.int_list",std::list<int>{1,2},"systrem int list");

mingfwq::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
	mingfwq::Config::Lookup("system.int_set",std::set<int>{1,2},"systrem int set");

mingfwq::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
	mingfwq::Config::Lookup("system.int_uset",std::unordered_set<int>{1,2},"systrem int uset");

mingfwq::ConfigVar<std::map<std::string,int>>::ptr g_str_int_map_value_config =
	mingfwq::Config::Lookup("system.str_int_map",std::map<std::string,int>{{"k",2}},"systrem str int map");

mingfwq::ConfigVar<std::unordered_map<std::string,int>>::ptr g_str_int_umap_value_config =
	mingfwq::Config::Lookup("system.str_int_umap",std::unordered_map<std::string,int>{{"k",2}},"systrem str int umap");

//对yaml的基本用法  遍历出来
//3是map 4是sequence 2是string
void print_yaml(const YAML::Node& node,int level){
    if(node.IsScalar()){
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << std::string(level * 4 , ' ')
            << node.Scalar() <<" - "<<node.Type() <<" - "<< level;
    }else if (node.IsNull()){
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << std::string(level * 4 , ' ')
            << "NULL - "<<node.Type() <<" - "<< level;
    }else if (node.IsMap()){
        for(auto it = node.begin();it != node.end(); ++it){
            MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << std::string(level * 4 , ' ')
                << it->first <<" - "<< it->second.Type() <<" - "<< level;
            print_yaml(it->second,level+1);
        }
    }else if (node.IsSequence()){
       for(size_t i = 0; i != node.size(); ++i){
            MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << std::string(level * 4 , ' ')
                << i <<" - "<< node[i].Type() <<" - "<< level;
            print_yaml(node[i],level+1);
        }
    }

}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/mingyue/c++/mingfwq/bin/conf/test.yml");
    print_yaml(root,0);


    MINGFWQ_LOG_INFO( MINGFWQ_LOG_ROOT()) << root.Scalar();
}

void test_config(){
    MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<< "before: "<< g_int_value_config -> getValue();
    //MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<< "before: "<< g_float_value_config -> toString();

//顺序容器可以用这个宏输出
#define XX(g_var, name ,prefix) \
    {\
        auto& v = g_var->getValue(); \
        for(auto& i : v){   \
            MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<<#prefix " " #name ": " << i; \
        }\
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<<#prefix " " #name " yaml: " << g_var->toString(); \
    }
//map,unordered_map
#define XX_M(g_var, name ,prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v){   \
            MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<<#prefix " " #name ": {" \
            << i.first << " - " << i.second << "}";  \
        } \
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<<#prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vector_value_config , int_vec, before);
    XX(g_int_list_value_config , int_list, before );
    XX(g_int_set_value_config , int_set, before );
    XX(g_int_uset_value_config , int_uset, before );
    XX_M(g_str_int_map_value_config , str_int_map, before );
    XX_M(g_str_int_umap_value_config , str_int_umap, before );


    //加载进来
    YAML::Node root = YAML::LoadFile("/home/mingyue/c++/mingfwq/bin/conf/test.yml");
    mingfwq::Config::LoadFromYaml(root);

    MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<< "after: "<< g_int_value_config -> getValue();
    //MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() )<< "after: "<< g_float_value_config -> toString();

    XX(g_int_vector_value_config , int_vec, after);
    XX(g_int_list_value_config , int_list, aftre);
    XX(g_int_set_value_config , int_set, aftre);
    XX(g_int_uset_value_config , int_uset, aftre);
    XX_M(g_str_int_map_value_config , str_int_map, after );
    XX_M(g_str_int_umap_value_config , str_int_umap, after );
}




//自定义类型查找
class Person{
public:
    std::string m_name ;
    int m_age = 0;
    bool m_sex = 0;
    std::string toString() const{
        std::stringstream ss;
        ss  << "[Person name=" <<m_name
            << " age=" <<m_age 
            << " sex=" <<m_sex
            << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const{
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};
namespace mingfwq{
template<>
class LexicalCast<std::string, Person >{
public:
	Person operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
		return p;
	}
};

template<>
class LexicalCast< Person, std::string >{
public:
	std::string operator() (const Person& p){
		YAML::Node node;
		node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

}

mingfwq::ConfigVar<Person>::ptr g_person =
	mingfwq::Config::Lookup("class.person",Person(),"systrem person");

mingfwq::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
	mingfwq::Config::Lookup("class.map",std::map<std::string, Person>(),"systrem person map");

mingfwq::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
	mingfwq::Config::Lookup("class.vec_map",std::map<std::string, std::vector<Person>>(),"systrem person map");


void test_class(){
    //MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << "before:" << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_var->getValue(); \
        for(auto& i : m){ \
            MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << prefix << ": "<< i.first << " - " \
            << i.second.toString(); \
        } \
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << prefix << ": size=" << m.size(); \
    }

    g_person->addListener([](const Person& old_value,const Person& new_value){
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT()) << "old value = "<< old_value.toString()
        <<" new value ="<<new_value.toString();
    });


    XX_PM(g_person_map,"class.map before");

    MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/mingyue/c++/mingfwq/bin/conf/test.yml");
    mingfwq::Config::LoadFromYaml(root);

    //MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << "after:" << g_person->getValue().toString() << " - " << g_person->toString();
    
    XX_PM(g_person_map,"class.map after");
    MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
    
}


/*

//练习
class Student{
public:
    std::string m_name ;
    int m_age = 0;
    int m_height = 0;
    bool m_sex = 0;
    std::string toString() const{
        std::stringstream ss;
        ss  << "[Person name=" <<m_name
            << " age=" <<m_age
            << " height="<<m_height 
            << " sex=" <<m_sex
            << "]";
        return ss.str();
    }
};
namespace mingfwq{
template<>
class LexicalCast<std::string, Student >{
public:
	Student operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        Student p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_height = node["height"].as<int>();
        p.m_sex = node["sex"].as<bool>();
		return p;
	}
};
template<>
class LexicalCast< Student, std::string >{
public:
	std::string operator() (const Student& p){
		YAML::Node node;
		node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["height"] = p.m_height;
        node["sex"] = p.m_sex;
        
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

}
mingfwq::ConfigVar<std::map<std::string, Student>>::ptr g_student_map =
	mingfwq::Config::Lookup("map.student",std::map<std::string, Student>(),"systrem student map");

void test_class_student(){
    MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT()) << "before: " << g_student_map->toString();

    YAML::Node root = YAML::LoadFile("/home/mingyue/c++/mingfwq/bin/conf/test.yml");
    mingfwq::Config::LoadFromYaml(root);
    
    MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT()) << "after: " << g_student_map->toString();

    //Configvar<map>::ptr （name,val,description）
    auto m = g_student_map->getValue(); 
    for(auto& i : m){ 
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << "after02: "<< i.first << " - " 
        << i.second.toString(); 
    } 
}
*/

void test_log(){
    static mingfwq::Logger::ptr system_log = MINGFWQ_LOG_NAME("system");
    std::cout << system_log->toYamlString() << std::endl;
    MINGFWQ_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout <<"======================"<<std::endl;
    std::cout << mingfwq::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/mingyue/c++/mingfwq/bin/conf/log.yml"); 
    mingfwq::Config::LoadFromYaml(root);
    std::cout <<"======================"<<std::endl;
    std::cout << mingfwq::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout <<"======================"<<std::endl;
    std::cout << root << std::endl;

    MINGFWQ_LOG_INFO(system_log) << "hello system" << std::endl;
    system_log ->setFormatter("%d - %m%n");
    MINGFWQ_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main(int agrc,char** argv){
    
    test_log();
    //test_class_student();
    //test_class();
    //test_config();
    //test_yaml();

    mingfwq::Config::Visit([](mingfwq::ConfigVarBase::ptr var){
        MINGFWQ_LOG_INFO(MINGFWQ_LOG_ROOT() ) << "name =" << var->getName()
                                << " description=" << var->getDescription()
                                << " typename=" << var->getTypeName()
                                << " value=" << var->toString();
    });


    return 0;
}
