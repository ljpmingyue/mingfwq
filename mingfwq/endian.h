#pragma once


#define MINGFWQ_LITTLE_ENDIAN 1
#define MINGFWQ_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>


/*  负责字节序的转换
**  因为网络字节序是 Big-Endian模式(标准), 
**  而大部分Windows系统都是 Little Endian模式
**  MINGFWQ_BYTE_ORDER 机器的字节序
*/
namespace mingfwq{
//8字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value){
    return (T)bswap_64((uint64_t)value);
}
//4字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value){
    return (T)bswap_32((uint32_t)value);
}
//2字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value){
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define MINGFWQ_BYTE_ORDER MINGFWQ_BIG_ENDIAN
#else 
#define MINGFWQ_BYTE_ORDER MINGFWQ_LITTLE_ENDIAN
#endif


#if MINGFWQ_BYTE_ORDER == MINGFWQ_BIG_ENDIAN
template<class T>
//只在小端机器上执行byteswap,在大端的机器上不操作
T byteswapOnLittleEndian(T t){
    //std::cout<< "----" << std::endl;
    return t;
}
//只在大端机器上执行byteswap，在小端的机器上不操作
template<class T>
T byteswapOnBigEndian(T t){
    return byteswap(t);
}
#else
template<class T>
T byteswapOnLittleEndian(T t){
    return byteswap(t);
}

template<class T>
T byteswapOnBigEndian(T t){
    return t;
}
#endif

}

