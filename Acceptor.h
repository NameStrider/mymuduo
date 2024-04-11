#ifndef __ACCEPTOR_H_
#define __ACCEPTOR_H_

#include <functional>
#include "Channel.h"
#include "Socket.h"
#include "noncopyable.h"

class EventLoop;
class InetAddress;

class Acceptor : public noncopyable {
public:
	typedef std::function<void(int, const InetAddress&)> NewConnectionCallback;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
	~Acceptor();

	void setNewConnectionCallback(const NewConnectionCallback& cb) {
		m_newConnectionCallback = cb;
	}

	bool listenning() const { return m_listenning; }
	void listen();
private:
	void handleRead();

	Socket m_acceptSocket;
	Channel m_acceptChannel;
	EventLoop* m_loop;
	NewConnectionCallback m_newConnectionCallback;

	bool m_listenning;
};

#endif // !__ACCEPTOR_H_
