#ifndef __SOCKET_H_
#define __SOCKET_H_

#include "noncopyable.h"

class InetAddress;

class Socket : public noncopyable {
public:
	explicit Socket(int sockfd) : m_sockfd(sockfd) {}
	~Socket();

	//封装了 bind, listen, accept 操作
	int fd() const { return m_sockfd; }
	void bindAddress(const InetAddress& localAddr);
	void listen();
	int accept(InetAddress* peerAddr);

	//对socket的选项设置 setsockopt()
	void shutdownWrite();
	void setTcpNoDelay(bool on);
	void setReuseAddr(bool on);
	void setReusePort(bool on);
	void setKeepAlive(bool on);
private:
	const int m_sockfd;
};

#endif // !__SOCKET_H_
