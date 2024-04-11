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
 * ������ָ�룬ע�� std::shared_ptr �� std::weak_ptr
 * ʲôʱ����ã����������ӽ��� TcpConnection ��ʱ���һ��Channel��m_tieָ����TcpConnection����
 */
void Channel::tie(const std::shared_ptr<void>& obj) {
	m_tie = obj;
	m_tied = true;
}

//����Poller�е��ļ���������Ӧ���¼����ͣ�Channel��Poller���ڲ�ͬģ�飬����ͨ��EventLoopͨ��
void Channel::update() {
	m_loop->updateChannel(this);
}

//��Channel������EventLoop��ɾ����Channel�������Լ�ɾ���Լ�
void Channel::remove() {
	m_loop->removeChanell(this);
}

//�ӿڣ�����Poller���ص��¼�
void Channel::handleEvent(TimeStamp timestamp) {
	std::shared_ptr<void> guard = m_tie.lock(); //�� weak_ptr תΪ shared_ptr
	if (m_tied) {	
		if (guard) {
			handleEventWithGuad(timestamp);
		}
	}
	else {
		handleEventWithGuad(timestamp);
	}
}

//��������Poller���ص��¼�
void Channel::handleEventWithGuad(TimeStamp recvtime) {
	//1.���Poller���ص��¼��Ƿ�����
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

	//2.����������IO�¼�
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