#include <errno.h>
#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include "Logger.h"
#include "Acceptor.h"
#include "InetAddress.h"

//����listenfd�Է�����ģʽ
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
	  m_acceptChannel(loop, m_acceptSocket.fd()) //��listenfd�󶨵�һ��Channel
{
	m_acceptSocket.setReuseAddr(true);
	m_acceptSocket.setReusePort(true);
	m_acceptSocket.bindAddress(listenAddr);

	//ע��Channel�Ķ��ص�������Acceptor::handleRead �������������µ���������ĺ���
	//mainloop[listenfd->request->connfd->chennel->subloop->thread]
	m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
	//1.Channel����ֹͣ�������ж���
	m_acceptChannel.disableAll();

	//2.�Ƴ�Channel����ļ��Ӷ���
	m_acceptChannel.remove();
}

//�����µ���������
void Acceptor::handleRead() {
	InetAddress peerAddr; //��¼�ͻ��˵�IP��Port��Ϣ

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
	//1.����m_listenning��־Ϊtrue
	m_listenning = true;

	//2.����listenfd
	m_acceptSocket.listen();

	//3.��Channel��ʹ�ܶ�������Poller���Լ���
	m_acceptChannel.enableReading();
}