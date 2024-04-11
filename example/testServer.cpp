#include <string>
#include "Logger.h"
#include "TcpServer.h"

class EchoServer {
public:
	EchoServer(EventLoop* loop,
		const std::string& name,
		const InetAddress& localAddr)
		: m_loop(loop),
		  m_server(loop, localAddr, name)
	{
		//注册回调函数
		m_server.setConnectionCallback(
			std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
		);
		m_server.setMessageCallback(
			std::bind(&EchoServer::onMessage, this, 
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
		);

		//设置线程个数
		m_server.setThreadNum(3); //线程个数与CPU核心数量相等时效率最高
	}

	void start() {
		m_server.start();
	}

private:
	//用户设置的连接或者断开回调
	void onConnection(const TcpConnectionPtr& conn) {
		if (conn->connected()) {
			LOG_INFO("connection eatablished: %s\n", conn->peerAddr().toPortIP().c_str());
		}
		else {
			LOG_INFO("connection disconected: %s\n", conn->peerAddr().toPortIP().c_str());
		}
	}

	//用户设置的IO就绪的回调
	void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, TimeStamp time) {
		std::string msg = buffer->retriveAllAsString();
		conn->send(msg);
		conn->shutdown(); //关闭写端，EPOLLHUP => CloseCallback
	}

	EventLoop* m_loop;
	TcpServer m_server;
};

int main() {
	//1.定义baseLoop
	EventLoop loop;

	//2.初始化本地ip端口号
	InetAddress localAddr("127.0.0.1", 8080); //默认 127.0.0.1:8080
	std::string name("EchoServer 1 ");

	//3.定义TcpServer
	EchoServer server(&loop, name ,localAddr);
	
	//4.启动事件循环
	server.start(); //listen in subLoop => acceptChannel => mainLoop
	loop.loop(); //启动mainLoop底层的Poller

	return 0;
}