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
	//打包成Channel，设置回调方法
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

//TcpConnection中动态分配的对象 m_socket 和 m_channel 由智能指针管理，无需手动释放
TcpConnection::~TcpConnection() {
	LOG_INFO("TcpConnection::dtor[%s] at fd:%d, state:%d\n", m_name.c_str(), m_channel->fd(), (int)m_state);
}

void TcpConnection::handleRead(TimeStamp receiveTime) {
	int savedErrno = 0;

	//从Buffer中读取数据
	ssize_t n = m_inputBuffer.readFd(m_channel->fd(), &savedErrno); //readFd会修改savedErrno

	//根据返回值判断错误状态，注意epoll工作在LT模式，返回-1就是有错误
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

			//全部发送完了
			if (m_outputBuffer.readableBytes() == 0) {
				m_channel->disableWriting();
				if (m_WriteCompleteCallback) {

					//通知loop所在的子线程执行参数的回调(执行这个函数的线程就是对应的子线程)
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
	m_ConnectionCallback(connptr); //通知用户连接关闭
	m_CloseCallback(connptr); //执行的是TcpServer::removeConnection
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
	//1.设置状态
	setState(kConnected);

	//2.绑定Channel
	m_channel->tie(shared_from_this());

	//3.使能读事件，Poller可以监视EPOLLIN类型事件
	m_channel->enableReading();

	//4.执行用户设置的回调
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
	if (m_state == kConnected) { //检查连接状态
		if (m_loop->isInLoopThread()) { //是否位于loop所在的线程
			sendInLoop(message.c_str(), message.size());
		}
		else { //否，当前线程注册回调并唤醒loop所在的线程通知其执行回调
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

	//错误状态，返回
	if (m_state == kDisconnected) {
		LOG_ERROR("disconnected, give up writing\n");
		return;
	}

	//channel第一次写或者写缓冲区没有可发送的数据
	if (!m_channel->isWritingEvent() && m_outputBuffer.readableBytes() == 0) {
		nwrite = ::write(m_channel->fd(), message, len);
		if (nwrite >= 0) {
			remaining = len - nwrite;

			//已经全部发送完，Channel就不再关注EPOLLOUT事件类型了
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
	 * 没有故障且 len 个字节没有发送完，需要把剩余的数据保存到写缓冲区，然后给Channel注册EPOLLOUT
	 * 事件，Poller将监视该事件类型并通知Channel
	 */
	if (!faultError && remaining > 0) {

		//发送缓冲区剩余的待发送数据
		size_t oldLen = m_outputBuffer.readableBytes(); 
		if (oldLen + remaining >= m_highWaterMark 
			&& oldLen < m_highWaterMark
			&& m_HighWaterMarkCallback) {
			m_loop->queueInLoop(
				std::bind(m_HighWaterMarkCallback, shared_from_this(), oldLen + remaining)
			);
		}

		/**
		 * 将本次没发送完的数据附加到写缓冲区，并且注册Channel的写事件。这样 Poller 就会监视内核的
		 * 发送缓冲，又因为epoll是LT模式，只要还有一个字节没发送就会通知应用进程，然后调用Channel
		 * 注册的写回调，也就是调用 TcpConnection::handleWrite 直到全部发送完
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