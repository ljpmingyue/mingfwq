#pragma once

namespace mingfwq{

class Noncopyable{
public:
    //默认构造函数
    Noncopyable() = default;
    //默认析构函数
    ~Noncopyable() = default;
    //拷贝构造函数（禁用）
    Noncopyable(const Noncopyable&) = delete;
    //赋值构造函数（禁用）
    Noncopyable& operator=(const Noncopyable&) = delete;
};



}