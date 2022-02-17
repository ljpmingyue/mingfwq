#include "../mingfwq/mingfwq.h"


static mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();



void test(){
#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec;\
    for (int i = 0; i < len; ++i){\
        vec.push_back(rand());\
    }\
    mingfwq::ByteArray::ptr ba(new mingfwq::ByteArray(base_len));\
    for(auto& j : vec){\
        ba->write_fun(j);\
    }\
    ba->setPosition(0);\
    for (size_t i = 0; i < vec.size(); ++i){\
        type v = ba->read_fun();\
        MINGFWQ_ASSERT(v == vec[i]);\
    }\
    MINGFWQ_ASSERT(ba->getReadSize() == 0);\
    MINGFWQ_LOG_INFO(g_logger) << #write_fun "/" #read_fun " (" #type " ) len = " << len \
                << " base_len = " << base_len << " size = " << ba->getSize();\
    }
    
    XX(int8_t, 10, writeFint8, readFint8, 1);
    XX(uint8_t, 10, writeFuint8, readFuint8, 1);
    XX(int16_t, 10, writeFint16, readFint16, 1);
    XX(uint16_t, 10, writeFuint16, readFint16, 1);
    XX(int32_t, 10, writeFint32, readFint32, 1);
    XX(uint32_t, 10, writeFuint32, readFint32, 1);
    XX(int64_t, 10, writeFint64, readFint64, 1);
    XX(uint64_t, 10, writeFuint64, readFint64, 1);
    
    XX(int32_t, 10, writeInt32, readInt32, 1);
    XX(uint32_t, 10, writeUint32, readUint32, 1);
    XX(int64_t, 10, writeInt64, readInt64, 1);
    XX(uint64_t, 10, writeUint64, readUint64, 1); 
 
#undef XX
}

void test_file(){
#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec;\
    for (int i = 0; i < len; ++i){\
        vec.push_back(rand());\
    }\
    mingfwq::ByteArray::ptr ba(new mingfwq::ByteArray(base_len));\
    for(auto& j : vec){\
        ba->write_fun(j);\
    }\
    ba->setPosition(0);\
    for (size_t i = 0; i < vec.size(); ++i){\
        type v = ba->read_fun();\
        MINGFWQ_ASSERT(v == vec[i]);\
    }\
    MINGFWQ_ASSERT(ba->getReadSize() == 0);\
    MINGFWQ_LOG_INFO(g_logger) << #write_fun "/" #read_fun " (" #type " ) len = " << len \
                << " base_len = " << base_len << " size = " << ba->getSize();\
    ba->setPosition(0);\
    MINGFWQ_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));\
    mingfwq::ByteArray::ptr ba2(new mingfwq::ByteArray(base_len * 2));\
    MINGFWQ_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    ba2->setPosition(0);\
    MINGFWQ_ASSERT(ba->getPosition() == 0);\
    MINGFWQ_ASSERT(ba2->getPosition() == 0);\
}
    XX(int8_t, 10, writeFint8, readFint8, 1);
    XX(uint8_t, 10, writeFuint8, readFuint8, 1);
    XX(int16_t, 10, writeFint16, readFint16, 1);
    XX(uint16_t, 10, writeFuint16, readFint16, 1);
    XX(int32_t, 10, writeFint32, readFint32, 1);
    XX(uint32_t, 10, writeFuint32, readFint32, 1);
    XX(int64_t, 10, writeFint64, readFint64, 1);
    XX(uint64_t, 10, writeFuint64, readFint64, 1);
    
    XX(int32_t, 10, writeInt32, readInt32, 1);
    XX(uint32_t, 10, writeUint32, readUint32, 1);
    XX(int64_t, 10, writeInt64, readInt64, 1);
    XX(uint64_t, 10, writeUint64, readUint64, 1); 
 
#undef XX 



}


int main(){
    //test();
    test_file();
    return 0;
}