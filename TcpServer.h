#ifndef __TCPSERVER_H_
#define __TCPSERVER_H_

#include <atomic>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

#include "Buffer.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"

class TcpServer : noncopyable {
public:
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	enum Option {
		kNoReusePort,
		kReusePort,
	};

	TcpServer(EventLoop* loop, const InetAddress& listenAddr, 
		const std::string& name, Option option = kNoReusePort);
	~TcpServer();

	void setThreadNum(int numThreads); //设置subLoop的个数

	void setConnectionCallback(const ConnectionCallback& cb) { m_ConnectionCallback = cb; }
	void setMessageCallback(const MessageCallback& cb) { m_MessageCallback = cb; }
	void setThreadInitCallback(const ThreadInitCallback& cb) { m_threadInitCallback = cb; }
	void setCloseCallback(const CloseCallback& cb) { m_CloseCallback = cb; }
	void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_WriteCompleteCallback = cb; }

	void start();
private:
	typedef std::unordered_map<std::string, TcpConnectionPtr> ConnectionMap;

	void newConnection(int sockfd, const InetAddress& peerAddr);
	void removeConnection(const TcpConnectionPtr& conn);
	void removeConnectionInLoop(const TcpConnectionPtr& conn);

	EventLoop* m_loop; //baseloop，用户自定义loop

	const std::string m_ipPort;
	const std::string m_name;

	std::unique_ptr<Acceptor> m_acceptor; //mainLoop运行，监听新连接
	std::shared_ptr<EventLoopThreadPool> m_threadPool; //one loop per thread

	ConnectionCallback m_ConnectionCallback; //新的连接请求的回调
	MessageCallback m_MessageCallback; //IO处理的回调
	CloseCallback m_CloseCallback; //断开连接的回调
	WriteCompleteCallback m_WriteCompleteCallback; //写完成的回调
	ThreadInitCallback m_threadInitCallback; //loop线程初始化的回调

	std::atomic_int m_started;
	int m_nextConnId;

	ConnectionMap m_connections; //记录所有的连接
};

#endif // !__TCPSERVER_H_
