#include "../mingfwq/mingfwq.h"
#include "../mingfwq/iomanager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>

mingfwq::Logger::ptr g_logger = MINGFWQ_LOG_ROOT();

int sock = 0;

void test_fiber(){
	MINGFWQ_LOG_INFO(g_logger) << "test_fiber";

	/*	两种类型的套接字：基于文件的和面向网络的
	**	基于文件的,家族名：AF_UNIX
	**	面向网络的,家族名：AF_INET
	**
	**	面向连接的套接字,TCP套接字的名字SOCK_STREAM。
	**	特点：可靠，开销大。
	**	UDP套接字的名字SOCK_DGRAM
	**	特点：不可靠（局网内还是比较可靠的），开销小。
	*/
	//打开的socket设为非阻塞的,用fcntl(socket, F_SETFL, O_NONBLOCK);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock, F_SETFL, O_NONBLOCK);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	/*	
	**	int inet_pton(int af, const char *src, void *dst);
	**	将点分十进制的ip地址转化为用于网络传输的数值格式，第一个参数af是地址簇，
	**	第二个参数*src是来源地址，第三个参数* dst接收转换后的数据
	**	返回值：若成功则为1，若输入不是有效的表达式则为0，若出错则为-1
	*/
	inet_pton(AF_INET, "14.215.177.38", &addr.sin_addr.s_addr);
	
	if( !connect(sock, (const sockaddr*)&addr, sizeof(addr)) ){
	} else if(errno == EINPROGRESS){
		MINGFWQ_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);

		mingfwq::IOManager::GetThis()->addEvent(sock, mingfwq::IOManager::READ, [](){
			MINGFWQ_LOG_INFO(g_logger) << "read callback";
		});

		mingfwq::IOManager::GetThis()->addEvent(sock, mingfwq::IOManager::WRITE, [](){
			MINGFWQ_LOG_INFO(g_logger) << "write callbake";
		
		//主动关闭sock不会触发read的回调函数
		//close(sock);
		mingfwq::IOManager::GetThis()->cancelEvent(sock, mingfwq::IOManager::READ);
		close(sock);
		});

	}else{
		MINGFWQ_LOG_INFO(g_logger) << "else " << errno << " "<< strerror(errno);
	}
	
}

void test1(){
	std::cout << "EPOLLIN = " << EPOLLIN 
			  << " EPOLLOUT = " << EPOLLOUT
			  << std::endl;
	//mingfwq::IOManager iom;
	mingfwq::IOManager iom(2, false);
	iom.schedule(&test_fiber);

}



mingfwq::Timer::ptr s_timer;

void test2(){
	mingfwq::IOManager iom(2);
	//循环定时器
	//mingfwq::Timer::ptr s_timer = iom.addTimer(1000, [](){
	//这样的s_timer没有初始化 
	s_timer = iom.addTimer(1000, [](){
		MINGFWQ_LOG_INFO(g_logger) << "hello timer循环";
			static int i = 0;
			if(++i == 5){
				s_timer->reset(2000,true); 
				//s_timer->cancel();
			}
			},true);


}



int main(){
	//test1();
	test2();


	
	return 0;
}
