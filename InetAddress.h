#ifndef __INETADDRESS_H_
#define __INETADDRESS_H_

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//struct sockaddr_in管理类
class InetAddress {
public:
	explicit InetAddress(const sockaddr_in& addr) : m_addr(addr) {}
	//默认参数的后面都应该是默认参数，否则编译器不知道怎么匹配
	explicit InetAddress(std::string ip = "127.0.0.1", uint16_t port = 8080);

	std::string toIP() const;
	std::string toPort() const;
	std::string toPortIP() const;
	const sockaddr_in* getSockAddr() const { return &m_addr; }
	void setSockAddr(const sockaddr_in& addr);
private:
	struct sockaddr_in m_addr;
};

#endif // !__INETADDRESS_H
