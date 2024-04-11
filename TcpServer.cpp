#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <functional>
#include "Logger.h"
#include "TcpServer.h"
#include "TcpConnection.h"

static EventLoop* checkLoopNotNULL(EventLoop* loop) {
	if (!loop) {
		LOG_FATAL("%s %s, baseLoop is NULL\n", __FILE__, __FUNCTION__);
	}
	return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr,
	const std::string& name, Option option)
	: m_loop(checkLoopNotNULL(loop)),
	  m_started(0),
	  m_ipPort(listenAddr.toPortIP()),
	  m_name(name),
	  m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)), //baseLoop
	  m_threadPool(new EventLoopThreadPool(loop, m_name)),
	  m_ConnectionCallback(),
	  m_MessageCallback(),
	  m_nextConnId(1)
{
	//��������ʱ����TcpServer::newConnection
	m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, 
		std::placeholders::_1, std::placeholders::_2)); //??std::placeholders
}

TcpServer::~TcpServer() {
	for (auto& item : m_connections) {

		//����ֲ��� shared_ptr ����ָ���� '}' ��Ϳ����ͷ� new ������ TcpConnection
		TcpConnectionPtr conn(item.second);
		item.second.reset(); //����ָ�����󶨲��ٹ������

		//�� map �м�¼�� TcpConnection ����������loop���Ƴ�
		conn->getLoop()->runInLoop(
			std::bind(&TcpConnection::connectDistroyed, conn)
		);
	}
}

void TcpServer::setThreadNum(int numThreads) {
	m_threadPool->setThreadNum(numThreads);
}

void TcpServer::start() {
	if (m_started++ == 0) { //��ֹһ��TcpServer��start���

		//�������������߳�
		m_threadPool->start(m_threadInitCallback);

		m_loop->runInLoop(
			std::bind(&Acceptor::listen, m_acceptor.get())
		);
	}
}

//��������ʱ��Acceptor������������
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
	EventLoop* ioLoop = m_threadPool->getNextLoop();
	char buffer[64] = { 0 };
	snprintf(buffer, sizeof(buffer), "-%s#%d", m_ipPort.c_str(), m_nextConnId);
	++m_nextConnId;
	std::string connName = m_name + buffer;

	LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
		m_name.c_str(), connName.c_str(), peerAddr.toPortIP().c_str());

	//��ȡsockfd�󶨵�ip�Ͷ˿ں�
	sockaddr_in local;
	memset(&local, 0, sizeof(local));
	socklen_t addrLen = sizeof(local);
	if (::getsockname(sockfd, (sockaddr*)&local, &addrLen) < 0) {
		LOG_ERROR("error in TcpServer::newConnection\n");
	}

	InetAddress localAddr(local);
	TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
	m_connections[connName] = conn;

	//��Щ�ص����������û����� TcpServer => TcpConnection => Channel => Poller => notify Channel
	conn->setConnectionCallback(m_ConnectionCallback);
	conn->setMessageCallback(m_MessageCallback);
	conn->setWriteCompleteCallback(m_WriteCompleteCallback);

	//�ر����ӵĻص�
	conn->setCloseCallback(
		std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
	);

	//����Channel��Poller׼�����ӣ�loop�������������߳�ִ��loop
	ioLoop->runInLoop(
		std::bind(&TcpConnection::connectEstablished, conn)
	);
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
	//baseLoopִ��TcpServer::removeConnectionInLoop
	m_loop->runInLoop(
		std::bind(&TcpServer::removeConnectionInLoop, this, conn)
	);
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
	LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", 
		m_name.c_str(), conn->name().c_str());
	m_connections.erase(conn->name());

	EventLoop* ioLoop = conn->getLoop(); //��ȡ����TcpConnection������loop
	ioLoop->queueInLoop(
		std::bind(&TcpConnection::connectDistroyed, conn) //??�е�ʱ����this���е�ʱ��������ָ��
	);
}