#ifndef __TCPCONNECTION_H_
#define __TCPCONNECTION_H_

#include <atomic>
#include <memory>
#include <string>
#include "Buffer.h"
#include "TimeStamp.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"

class Socket;
class Channel;
class EventLoop;

/**
 * TcpServer => Acceptor => new connection, connfd = accept() => TcpConnection, set callback
 * => Channel => Poller => subLoop
 */
class TcpConnection : public noncopyable, std::enable_shared_from_this<TcpConnection>
{
public:
	enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected };

	TcpConnection(EventLoop* loop, 
		const std::string& name, 
		int sockfd,
		const InetAddress& localAddr,
		const InetAddress& peerAddr);
	~TcpConnection();

	EventLoop* getLoop() const { return m_loop; }
	const std::string& name() const { return m_name; }
	const InetAddress& localAddr() const { return m_localAddr; }
	const InetAddress& peerAddr() const { return m_peerAddr; }

	bool connected() const{ return m_state == kConnected; }
	void send(const std::string& message);
	void setState(StateE state) { m_state = state; }
	void shutdown();

	void setConnectionCallback(const ConnectionCallback& cb) { m_ConnectionCallback = cb; }
	void setMessageCallback(const MessageCallback& cb) { m_MessageCallback = cb; }
	void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_WriteCompleteCallback = cb; }
	void setCloseCallback(const CloseCallback& cb) { m_CloseCallback = cb; }
	void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
		m_HighWaterMarkCallback = cb; 
		m_highWaterMark = highWaterMark;
	}

	void connectEstablished();
	void connectDistroyed();
private:
	void handleRead(TimeStamp receiveTime);
	void handleWrite();
	void handleClose();
	void handleError();

	//在loop中发送数据
	void sendInLoop(const void* message, size_t len);

	//在loop中关闭TcpConnection
	void shutdownInLoop();

	EventLoop* m_loop; //肯定不是mainLoop，因为mainLoop监视到新连接时会通知subLoop并由其执行
	const std::string m_name;
	std::atomic_int m_state;
	bool m_reading;
	size_t m_highWaterMark; //高水位？？

	std::unique_ptr<Socket> m_socket; //管理 socket, bind, listen, accept
	std::unique_ptr<Channel> m_channel;
	const InetAddress m_localAddr; //管理 struct sockaddr_in
	const InetAddress m_peerAddr;

	ConnectionCallback m_ConnectionCallback;
	MessageCallback m_MessageCallback;
	HighWaterMarkCallback m_HighWaterMarkCallback;
	WriteCompleteCallback m_WriteCompleteCallback;
	CloseCallback m_CloseCallback;

	Buffer m_inputBuffer;
	Buffer m_outputBuffer;
};

#endif // !__TCPCONNECTION_H_
