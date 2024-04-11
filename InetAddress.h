#ifndef __INETADDRESS_H_
#define __INETADDRESS_H_

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//struct sockaddr_in������
class InetAddress {
public:
	explicit InetAddress(const sockaddr_in& addr) : m_addr(addr) {}
	//Ĭ�ϲ����ĺ��涼Ӧ����Ĭ�ϲ����������������֪����ôƥ��
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
