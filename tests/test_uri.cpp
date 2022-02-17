#include "../mingfwq/uri.h"
#include "../mingfwq/log.h"
#include <iostream>


static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

int main(){

    //mingfwq::Uri::ptr uri = mingfwq::Uri::Create("http://www.sylar.top/test/uri?id=100&name=mingfwq$fragment");
    mingfwq::Uri::ptr uri = mingfwq::Uri::Create("http://www.sylar.top/test/中文/uri?id=100&name=mingfwq&vv=中文$frag中文");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;

}