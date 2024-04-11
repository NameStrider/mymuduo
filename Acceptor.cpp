#include <errno.h>
#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include "Logger.h"
#include "Acceptor.h"
#include "InetAddress.h"

//创建listenfd以非阻塞模式
static int createNonblocking() {
	int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (sockfd < 0) {
		LOG_FATAL("%s %s, create socket failed, errno:%d\n", __FILE__, __FUNCTION__, errno);
	}
	return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort) 
	: m_loop(loop),
	  m_listenning(false),
	  m_acceptSocket(createNonblocking()),
	  m_acceptChannel(loop, m_acceptSocket.fd()) //将listenfd绑定到一个Channel
{
	m_acceptSocket.setReuseAddr(true);
	m_acceptSocket.setReusePort(true);
	m_acceptSocket.bindAddress(listenAddr);

	//注册Channel的读回调方法，Acceptor::handleRead 就是真正处理新的连接请求的函数
	//mainloop[listenfd->request->connfd->chennel->subloop->thread]
	m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
	//1.Channel申请停止监视所有对象
	m_acceptChannel.disableAll();

	//2.移除Channel管理的监视对象
	m_acceptChannel.remove();
}

//处理新的连接请求
void Acceptor::handleRead() {
	InetAddress peerAddr; //记录客户端的IP和Port信息

	int connfd = m_acceptSocket.accept(&peerAddr);
	if (connfd >= 0) {
		if (m_newConnectionCallback) {
			m_newConnectionCallback(connfd, peerAddr);
		}
		else {
			::close(connfd);
		}
	}
	else {
		LOG_ERROR("%s %s, accept error, errno:%d\n", __FILE__, __FUNCTION__, errno);
	}
}

void Acceptor::listen() {
	//1.设置m_listenning标志为true
	m_listenning = true;

	//2.监听listenfd
	m_acceptSocket.listen();

	//3.在Channel中使能读就绪，Poller可以监视
	m_acceptChannel.enableReading();
}