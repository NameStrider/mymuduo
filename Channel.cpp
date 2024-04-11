#include <sys/epoll.h>
#include "EventLoop.h"
#include "Channel.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; //EPOLLPRI??
const int Channel::kWriteEvent = EPOLLOUT;

//EventLoop: ChannelList + Poller
Channel::Channel(EventLoop* loop, int fd) 
	: m_loop(loop), m_fd(fd), m_events(0), m_revents(0), m_index(-1), m_tied(false) {

}

Channel::~Channel() {}

/**
 * 绑定智能指针，注意 std::shared_ptr 和 std::weak_ptr
 * 什么时候调用？当有新连接建立 TcpConnection 的时候绑定一个Channel，m_tie指向了TcpConnection对象
 */
void Channel::tie(const std::shared_ptr<void>& obj) {
	m_tie = obj;
	m_tied = true;
}

//更新Poller中的文件描述符对应的事件类型，Channel和Poller属于不同模块，他们通过EventLoop通信
void Channel::update() {
	m_loop->updateChannel(this);
}

//在Channel所属的EventLoop中删除此Channel，不能自己删除自己
void Channel::remove() {
	m_loop->removeChanell(this);
}

//接口，处理Poller返回的事件
void Channel::handleEvent(TimeStamp timestamp) {
	std::shared_ptr<void> guard = m_tie.lock(); //将 weak_ptr 转为 shared_ptr
	if (m_tied) {	
		if (guard) {
			handleEventWithGuad(timestamp);
		}
	}
	else {
		handleEventWithGuad(timestamp);
	}
}

//真正处理Poller返回的事件
void Channel::handleEventWithGuad(TimeStamp recvtime) {
	//1.检查Poller返回的事件是否正常
	if ((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN)) { //EPOLLHUP??
		if (m_closeCallback) {
			m_closeCallback();
		}
	}
	if (m_revents & EPOLLERR) {
		if (m_errorCallback) {
			m_errorCallback();
		}
	}

	//2.正常，处理IO事件
	if (m_revents & (EPOLLIN | EPOLLPRI)) {
		if (m_readCallback) {
			m_readCallback(recvtime);
		}
	}
	if (m_revents & EPOLLOUT) {
		if (m_writeCallback) {
			m_writeCallback();
		}
	}
}