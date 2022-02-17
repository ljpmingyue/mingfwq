#pragma once

#include <memory>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

namespace mingfwq{
/*  二进制数组(序列化/反序列化)
**  为了后面做网络协议的解析，（网络字节序）
**  需要我们提供一个统一的做序列化和反序列化去取数据的类
*/
class ByteArray{
public:
    typedef std::shared_ptr<ByteArray> ptr;

    //链表结构体存储信息
    struct Node{
        Node(size_t s);
        Node();
        ~Node();

        //内存块地址指针
        char* ptr;
        //下一块内存地址指针
        Node* next;
        //大小
        size_t size;
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    //write
    //固定长度写int
    void writeFint8  (const int8_t& value);
    void writeFuint8 (const uint8_t& value);
    void writeFint16 ( int16_t value);
    void writeFuint16( uint16_t value);
    void writeFint32 ( int32_t value);
    void writeFuint32( uint32_t value);
    void writeFint64 ( int64_t value);
    void writeFuint64( uint64_t value);
    //不固定长度写int,压缩
    void writeInt32  ( int32_t value);
    void writeUint32 ( uint32_t value);
    void writeInt64  ( int64_t value);
    void writeUint64 ( uint64_t value);

    void writeFloat  ( float value);
    void writeDouble ( double value);
    //length: 16, data
    void writeStringF16(const std::string& value);
    //length: 32, data
    void writeStringF32(const std::string& value);
    //length: 64, data
    void writeStringF64(const std::string& value);
    //length: varint, data
    void writeStringFVint(const std::string& value);
    // data
    void writeStringFWithoutLength(const std::string& value);


    //read
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float  readFloat();
    double readBouble();

    //length:16,32,64,varint ;data
    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    //内部操作
    void clear();
    //size 指字节
    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position)const;
    
    //返回bytearray当前位置
    size_t getPosition()const{return m_position;}
    void setPosition(size_t v);

    //把bytearray写入或读出
    bool writeToFile(const std::string& name)const;
    bool readFromFile(const std::string& name);

    //返回内存块的大小
    size_t getBaseSize()const{return m_baseSize;}
    //还有多少可以读
    size_t getReadSize()const{return m_size - m_position;} 
    size_t getSize()const {return m_size;}

    bool isLittleEndian()const;
    void setIsLittleEndian(bool val);

    //文本形式输出
    std::string toString() const;
    //二进制形式输出
    std::string toHexString()const;

    //sock相关的, revcmsg,sendmsg    struct msghdr

    //  只获取内容不修改position
    /*  获取可读取的缓存,保存成iovec数组
    **  buffers 保存可读取数据的iovec数组，len 读取数据的长度
    **  返回实际数据的长度
    **/
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull)const;
    /*  获取可读取的缓存,保存成iovec数组,从position位置开始
    **  返回实际数据的长度
    */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position )const;

    //  增加容量，但是不修改position
    /*  获取可写入的缓存,保存成iovec数组，返回实际数据的长度
    **  如果(m_position + len) > m_capacity 则 m_capacity扩容N个节点以容纳len长度
    **/
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
private:
    //扩容ByteArray,使其可以容纳size个数据(如果原本可以可以容纳,则不扩容)
    void addCapacity(size_t size);
    //获取当前的可写入容量
    size_t getCapacity()const{return m_capacity - m_position;}

private:
    //内存块的大小(每一个node有多大)
    size_t m_baseSize;
    //当前操作位置
    size_t m_position;
    //当前数据的总容量
    size_t m_capacity;
    //当前真实数据大小
    size_t m_size;
    //字节序,默认大端
    int8_t m_endian;
    //第一个内存块的指针
    Node* m_root;
    //当前操作的内存块的指针
    Node* m_cur;

};


}