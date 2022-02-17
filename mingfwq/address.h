#pragma once

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

namespace mingfwq{

class IPAddress;
//网络地址的封装
class Address{
public:
    typedef std::shared_ptr<Address> ptr;
    /*
    **  通过sockaddr指针创建Address
    **  addr sockaddr指针,addrlen sockaddr的长度
    **  返回和sockaddr相匹配的Address,失败返回nullptr
    */
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
    /*
    **  通过host地址返回对应条件的所有Address
    **  result 保存满足条件的Address
    **  host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
    **  family 协议族(AF_INET, AF_INET6, AF_UNIX)
    **  type socketl类型SOCK_STREAM、SOCK_DGRAM 等
    **  protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
    **  return 返回是否转换成功
    */    
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                int family = AF_INET, int type = 0, int protocol = 0);
    //返回满足条件的第一个Address,失败返回nullptr
    static Address::ptr LookupAny(const std::string& host,
                int family = AF_INET, int type = 0, int protocol = 0);   
    //返回满足条件的第一IPAddress,失败返回nullptr
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                int family = AF_INET, int type = 0, int protocol = 0); 
    
    /*  返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
    **  result 保存本机所有地址
    **  family 协议族(AF_INT, AF_INT6, AF_UNIX)
    **  是否获取成功
    */
    static bool GetInterfaceAddresses(std::multimap<std::string , std::pair<Address::ptr, uint32_t>>& result,
                    int family = AF_INET, int type = 0 , int protocol = 0);
    /*  返回指定网卡的地址和子网掩码位数
    **  result 保存指定网卡所有地址
    **  iface 网卡名称 
    **  family 协议族(AF_INT, AF_INT6, AF_UNIX)
    **  是否获取成功
    */
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result,
                            const std::string& iface, int family = AF_INET);

    virtual ~Address(){}
    //返回协议簇
    int getFamily()const;
    //返回sockaddr指针,只读
    virtual const sockaddr* getAddr()const = 0;
    virtual sockaddr* getAddr() = 0;
    //返回sockaddr的长度
    virtual socklen_t getAddrLen() const = 0;

    //可读性输出地址
    virtual std::ostream& insert(std::ostream& os)const = 0;
    std::string toString()const;

    bool operator<(const Address& rhs)const;
    bool operator==(const Address& rhs)const;
    bool operator!=(const Address& rhs)const;


};

class IPAddress:public Address{
public:
    typedef std::shared_ptr<IPAddress> ptr;
    /**
    **  通过域名,IP,服务器名创建IPAddress
    **  address 域名,IP,服务器名等.举例: www.sylar.top
    **  port 端口号
    **  调用成功返回IPAddress,失败返回nullptr
    */
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    //获取该地址的广播地址, prefix_len：子网掩码数 成功ipaddress 失败nullptr
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    //获取该地址的网段
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    //获取子网掩码地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;
    //获得端口号
    virtual uint32_t getPort()const = 0;
    //设置端口号
    virtual void setPort(uint16_t v) = 0;

};

class IPv4Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv4Address> ptr;
    //INADDR_ANY：转换过来就是0.0.0.0，泛指本机的意思，也就是表示本机的所有IP
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    IPv4Address(const sockaddr_in& address);

    //使用点分十进制地址创建IPv4Address "192.168.1.1"
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    const sockaddr* getAddr()const override;
    sockaddr* getAddr()override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort()const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    static IPv6Address::ptr Create(const char* address , uint16_t port = 0);

    IPv6Address();
    IPv6Address(const uint8_t address[16], uint32_t);
    IPv6Address(const sockaddr_in6& address);
    

    const sockaddr* getAddr()const override;
    sockaddr* getAddr()override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;


    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort()const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;
};

class UnixAddress : public Address{
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* getAddr()const override;
    sockaddr* getAddr()override;
    void setAddrLen(uint32_t v);
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;
private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknowAddress: public Address{
public:
    typedef std::shared_ptr<UnknowAddress> ptr;
    UnknowAddress(int family);
    UnknowAddress(const sockaddr& addr);
    const sockaddr* getAddr()const override;
    sockaddr* getAddr()override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;

private:
    sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);


}