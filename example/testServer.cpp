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
		//ע��ص�����
		m_server.setConnectionCallback(
			std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
		);
		m_server.setMessageCallback(
			std::bind(&EchoServer::onMessage, this, 
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
		);

		//�����̸߳���
		m_server.setThreadNum(3); //�̸߳�����CPU�����������ʱЧ�����
	}

	void start() {
		m_server.start();
	}

private:
	//�û����õ����ӻ��߶Ͽ��ص�
	void onConnection(const TcpConnectionPtr& conn) {
		if (conn->connected()) {
			LOG_INFO("connection eatablished: %s\n", conn->peerAddr().toPortIP().c_str());
		}
		else {
			LOG_INFO("connection disconected: %s\n", conn->peerAddr().toPortIP().c_str());
		}
	}

	//�û����õ�IO�����Ļص�
	void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, TimeStamp time) {
		std::string msg = buffer->retriveAllAsString();
		conn->send(msg);
		conn->shutdown(); //�ر�д�ˣ�EPOLLHUP => CloseCallback
	}

	EventLoop* m_loop;
	TcpServer m_server;
};

int main() {
	//1.����baseLoop
	EventLoop loop;

	//2.��ʼ������ip�˿ں�
	InetAddress localAddr("127.0.0.1", 8080); //Ĭ�� 127.0.0.1:8080
	std::string name("EchoServer 1 ");

	//3.����TcpServer
	EchoServer server(&loop, name ,localAddr);
	
	//4.�����¼�ѭ��
	server.start(); //listen in subLoop => acceptChannel => mainLoop
	loop.loop(); //����mainLoop�ײ��Poller

	return 0;
}