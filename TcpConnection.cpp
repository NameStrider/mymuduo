#include <unistd.h>
#include <sys/types.h>     
#include <sys/socket.h>
#include <functional>
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "TcpConnection.h"

static EventLoop* checkLoopNotNULL(EventLoop* loop) {
	if (!loop) {
		LOG_FATAL("%s %s, TcpConnection loop is NULL\n", __FILE__, __FUNCTION__);
	}
	return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
	const std::string& name,
	int sockfd,
	const InetAddress& localAddr,
	const InetAddress& peerAddr)
	: m_loop(checkLoopNotNULL(loop)),
	  m_name(name),
	  m_localAddr(localAddr),
	  m_peerAddr(peerAddr),
	  m_reading(true),
	  m_state(kConnecting), //??
	  m_socket(new Socket(sockfd)),
	  m_channel(new Channel(loop, sockfd)),
	  m_highWaterMark(64 * 1024 * 1024) //64M
{
	//�����Channel�����ûص�����
	m_channel->setReadCallback(
		std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
	);

	m_channel->setWriteCallback(
		std::bind(&TcpConnection::handleWrite, this)
	);

	m_channel->setCloseCallback(
		std::bind(&TcpConnection::handleClose, this)
	);

	m_channel->setErrorCallback(
		std::bind(&TcpConnection::handleError, this)
	);

	LOG_INFO("TcpConnection::ctor[%s] at fd:%d\n", m_name.c_str(), sockfd);
	m_socket->setKeepAlive(true);
}

//TcpConnection�ж�̬����Ķ��� m_socket �� m_channel ������ָ����������ֶ��ͷ�
TcpConnection::~TcpConnection() {
	LOG_INFO("TcpConnection::dtor[%s] at fd:%d, state:%d\n", m_name.c_str(), m_channel->fd(), (int)m_state);
}

void TcpConnection::handleRead(TimeStamp receiveTime) {
	int savedErrno = 0;

	//��Buffer�ж�ȡ����
	ssize_t n = m_inputBuffer.readFd(m_channel->fd(), &savedErrno); //readFd���޸�savedErrno

	//���ݷ���ֵ�жϴ���״̬��ע��epoll������LTģʽ������-1�����д���
	if (n > 0) {
		m_MessageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
	}
	else if (n == 0) {
		handleClose();
	}
	else {
		errno = savedErrno;
		LOG_ERROR("error in TcpConnection::handleRead()");
		handleError();
	}
}

void TcpConnection::handleWrite() {
	if (m_channel->isWritingEvent()) {
		int savedError = 0;
		ssize_t n = m_outputBuffer.writefd(m_channel->fd(), &savedError);
		if (n > 0) {
			m_outputBuffer.retrive(n);

			//ȫ����������
			if (m_outputBuffer.readableBytes() == 0) {
				m_channel->disableWriting();
				if (m_WriteCompleteCallback) {

					//֪ͨloop���ڵ����߳�ִ�в����Ļص�(ִ������������߳̾��Ƕ�Ӧ�����߳�)
					m_loop->queueInLoop(
						std::bind(&TcpConnection::m_WriteCompleteCallback, shared_from_this())
					);
				}
				if (m_state == kDisconnecting) {
					shutdownInLoop();
				}
			}
		}
		else {
			LOG_ERROR("error in TcpConnection::handleWrite");
		}
	}
	else {
		LOG_ERROR("connection fd:%d is down, no more writing\n", m_channel->fd());
	}
}

//Poller => Channel::closeCallback => TcpConnection::handleClose
void TcpConnection::handleClose() {
	LOG_INFO("TcpConnection::handleClose, fd=%d, state=%d\n", m_channel->fd(), (int)m_state);
	setState(kDisconnected);
	m_channel->disableAll();

	TcpConnectionPtr connptr(shared_from_this());
	m_ConnectionCallback(connptr); //֪ͨ�û����ӹر�
	m_CloseCallback(connptr); //ִ�е���TcpServer::removeConnection
}

void TcpConnection::handleError() {
	int err;
	int optval;
	socklen_t optlen = sizeof(optval);

	if (::getsockopt(m_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
		err = errno;	
	}
	else {
		err = optval;
	}

	LOG_ERROR("TcpConnection::handleError name:%s SO_ERROR:%d\n", m_name.c_str(), err);
}

void TcpConnection::connectEstablished() {
	//1.����״̬
	setState(kConnected);

	//2.��Channel
	m_channel->tie(shared_from_this());

	//3.ʹ�ܶ��¼���Poller���Լ���EPOLLIN�����¼�
	m_channel->enableReading();

	//4.ִ���û����õĻص�
	m_ConnectionCallback(shared_from_this());
}

void TcpConnection::connectDistroyed() {
	if (m_state == kConnected) {
		setState(kDisconnected);
		m_channel->disableAll();
		m_ConnectionCallback(shared_from_this());
	}

	m_channel->remove();
}

void TcpConnection::send(const std::string& message) {
	if (m_state == kConnected) { //�������״̬
		if (m_loop->isInLoopThread()) { //�Ƿ�λ��loop���ڵ��߳�
			sendInLoop(message.c_str(), message.size());
		}
		else { //�񣬵�ǰ�߳�ע��ص�������loop���ڵ��߳�֪ͨ��ִ�лص�
			m_loop->runInLoop(
				std::bind(&TcpConnection::sendInLoop, 
					this,
					message.c_str(),
					message.size())
			);
		}
	}
}

void TcpConnection::sendInLoop(const void* message, size_t len) {
	ssize_t nwrite = 0;
	size_t remaining = len;
	bool faultError = false;

	//����״̬������
	if (m_state == kDisconnected) {
		LOG_ERROR("disconnected, give up writing\n");
		return;
	}

	//channel��һ��д����д������û�пɷ��͵�����
	if (!m_channel->isWritingEvent() && m_outputBuffer.readableBytes() == 0) {
		nwrite = ::write(m_channel->fd(), message, len);
		if (nwrite >= 0) {
			remaining = len - nwrite;

			//�Ѿ�ȫ�������꣬Channel�Ͳ��ٹ�עEPOLLOUT�¼�������
			if (remaining == 0 && m_WriteCompleteCallback) {
				m_loop->queueInLoop(
					std::bind(m_WriteCompleteCallback, shared_from_this())
				);
			}
		}
		else {
			nwrite = 0;
			if (errno != EWOULDBLOCK) {
				LOG_ERROR("error in TcpConnection::sendInLoop\n");
				if (errno == EPIPE || errno == ECONNRESET) { //SIGPIPE
					faultError = true;
				}
			}		
		}
	}

	/**
	 * û�й����� len ���ֽ�û�з����꣬��Ҫ��ʣ������ݱ��浽д��������Ȼ���Channelע��EPOLLOUT
	 * �¼���Poller�����Ӹ��¼����Ͳ�֪ͨChannel
	 */
	if (!faultError && remaining > 0) {

		//���ͻ�����ʣ��Ĵ���������
		size_t oldLen = m_outputBuffer.readableBytes(); 
		if (oldLen + remaining >= m_highWaterMark 
			&& oldLen < m_highWaterMark
			&& m_HighWaterMarkCallback) {
			m_loop->queueInLoop(
				std::bind(m_HighWaterMarkCallback, shared_from_this(), oldLen + remaining)
			);
		}

		/**
		 * ������û����������ݸ��ӵ�д������������ע��Channel��д�¼������� Poller �ͻ�����ں˵�
		 * ���ͻ��壬����Ϊepoll��LTģʽ��ֻҪ����һ���ֽ�û���;ͻ�֪ͨӦ�ý��̣�Ȼ�����Channel
		 * ע���д�ص���Ҳ���ǵ��� TcpConnection::handleWrite ֱ��ȫ��������
		 */
		m_outputBuffer.append((char*)message + nwrite, remaining);
		if (!m_channel->isWritingEvent()) {
			m_channel->enableWriting();
		}
	}
}

void TcpConnection::shutdown() {
	if (m_state == kConnected) {
		setState(kDisconnecting);
		m_loop->runInLoop(
			std::bind(&TcpConnection::shutdownInLoop, this)
		);
	}
}

void TcpConnection::shutdownInLoop() {
	if (!m_channel->isWritingEvent()) {
		m_socket->shutdownWrite();
	}
}