#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include "Logger.h"
#include "InetAddress.h"
#include "Socket.h"

//¹Ø±Õlistenfd
Socket::~Socket() {
	::close(m_sockfd);
}

void Socket::bindAddress(const InetAddress& localAddr) {
	int ret = ::bind(m_sockfd, (sockaddr*)localAddr.getSockAddr(), sizeof(sockaddr_in));
	if (ret < 0) {
		LOG_FATAL("bind sockfd %d failed\n", m_sockfd);
	}
}

void Socket::listen() {
	int ret = ::listen(m_sockfd, 1024);
	if (ret < 0) {
		LOG_FATAL("listen sockfd %d failed\n", m_sockfd);
	}
}

int Socket::accept(InetAddress* peerAddr) {
	sockaddr_in addr;
	socklen_t AddrLen = sizeof(addr);
	memset(&addr, 0, sizeof(sockaddr_in));

	int connfd = ::accept4(m_sockfd, (sockaddr*)&addr, &AddrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (connfd >= 0) {
		peerAddr->setSockAddr(addr);
	}

	return connfd;
}

void Socket::shutdownWrite() {
	int ret = ::shutdown(m_sockfd, SHUT_WR);
	if (ret < 0) {
		LOG_ERROR("shutdownWrite failed\n");
	}
}

void Socket::setTcpNoDelay(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int));
}
void Socket::setReuseAddr(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
}
void Socket::setReusePort(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int));
}
void Socket::setKeepAlive(bool on) {
	int optval = on ? 1 : 0;
	::setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int));
}