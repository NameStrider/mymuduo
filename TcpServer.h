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

	void setThreadNum(int numThreads); //����subLoop�ĸ���

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

	EventLoop* m_loop; //baseloop���û��Զ���loop

	const std::string m_ipPort;
	const std::string m_name;

	std::unique_ptr<Acceptor> m_acceptor; //mainLoop���У�����������
	std::shared_ptr<EventLoopThreadPool> m_threadPool; //one loop per thread

	ConnectionCallback m_ConnectionCallback; //�µ���������Ļص�
	MessageCallback m_MessageCallback; //IO����Ļص�
	CloseCallback m_CloseCallback; //�Ͽ����ӵĻص�
	WriteCompleteCallback m_WriteCompleteCallback; //д��ɵĻص�
	ThreadInitCallback m_threadInitCallback; //loop�̳߳�ʼ���Ļص�

	std::atomic_int m_started;
	int m_nextConnId;

	ConnectionMap m_connections; //��¼���е�����
};

#endif // !__TCPSERVER_H_
