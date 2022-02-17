#pragma once
#include "log.h"

#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>

namespace mingfwq{
    bool is_hook_enable();
    void set_hook_enable(bool flag);


}


//using recv_func = ssize_t (*)(int sockfd, void *buf, size_t len, int flags);

extern "C"{
    //如果想用老方法就用新定义的sleep_f和usleep_f
    //新定义就是sleep_fun和usleep_fun
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun  usleep_f;

    typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
    extern nanosleep_fun nanosleep_f;
    
    //socket
    typedef int (*socket_fun)(int domain, int type, int protocol);
    extern socket_fun socket_f;

    typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    extern connect_fun connect_f;

    extern int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,uint64_t timeout_ms);

    typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
    extern accept_fun accept_f;

    //read
    typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
    extern read_fun read_f;

    typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
    extern readv_fun readv_f;

    typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
    extern recv_fun recv_f;

    typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    extern recvfrom_fun recvfrom_f;

    typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
    extern recvmsg_fun recvmsg_f;

    //write
    typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
    extern write_fun write_f;

    typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
    extern writev_fun writev_f;

    typedef ssize_t  (*send_fun)(int s, const void *msg, size_t len, int flags);
    extern send_fun send_f;

    typedef ssize_t  (*sendto_fun)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
    extern sendto_fun sendto_f;

    typedef ssize_t  (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
    extern sendmsg_fun sendmsg_f;

    //socket的一些操作
    typedef int (*close_fun)(int fd);
    extern close_fun close_f;

    typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
    extern fcntl_fun fcntl_f;

    typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
    extern ioctl_fun ioctl_f;

    typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
    extern getsockopt_fun getsockopt_f;

    typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
    extern setsockopt_fun setsockopt_f;

/*  
sleep
    int usleep(useconds_t usec);
    unsigned int sleep(unsigned int seconds);
    int nanosleep(const struct timespec *req, struct timespec *rem);

socket
    int socket(int domain, int type, int protocol);
    //用来客户端连接服务器用的
    int connect(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen);
    //服务端创建新连接
    int accept(int s, struct sockaddr *addr, socklen_t *addrlen);

读相关 read readv recv recvfrom recvmsg
    ssize_t read(int fd, void *buf, size_t count);
    //
    ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
    //
    ssize_t recv(int sockfd, void *buf, size_t len, int flags);

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                struct sockaddr *src_addr, socklen_t *addrlen);

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
 
写相关 write writev send sendto sendmsg
    ssize_t write(int fd, const void *buf, size_t count);
    ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
    ssize_t  send(int s, const void *msg, size_t len, int flags);
    ssize_t  sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
    ssize_t  sendmsg(int s, const struct msghdr *msg, int flags);

socket的一些操作
    int close(int fd);
    可以用来对已打开的文件描述符进行各种控制操作以改变已打开文件的的各种属性
    int fcntl(int fd, int cmd, ... );
    int ioctl(int fd, unsigned long request, ...);
    int getsockopt(int sockfd, int level, int optname,
                void *optval, socklen_t *optlen);
    int setsockopt(int sockfd, int level, int optname,
                const void *optval, socklen_t optlen);

*/
}

    