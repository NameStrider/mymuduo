#include <string.h>
#include "InetAddress.h"

//通过ip和port构造对象
InetAddress::InetAddress(std::string ip, uint16_t port) {
	memset(&m_addr, 0, sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);
	m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

//返回IP字符串
std::string InetAddress::toIP() const {
	char buffer[64] = { 0 };
	inet_ntop(AF_INET, &m_addr.sin_addr, buffer, sizeof(buffer));
	return std::string(buffer);
}

//返回port字符串
std::string InetAddress::toPort() const {
	char buffer[10] = { 0 };
	uint16_t port = ntohs(m_addr.sin_port);
	snprintf(buffer, sizeof(buffer), "%u", port);
	return std::string(buffer);
}

//返回"IP:Port"字符串
std::string InetAddress::toPortIP() const {
	//ip:port
	std::string str;
	str.append(toIP());
	str.append(":");
	str.append(toPort());
	return str;
}

void InetAddress::setSockAddr(const sockaddr_in& addr) {
	m_addr = addr;
}

//#include <iostream>
//int main() {
//	InetAddress addr(std::string("127.0.0.1"), 8080);
//	std::cout << addr.toPortIP() << std::endl;
//	return 0;
//}