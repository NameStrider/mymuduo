#ifndef __EPOLLER_H_
#define __EPOLLER_H_

#include <vector>
#include <sys/epoll.h>
#include "Poller.h"

/**
 * Poller的派生类Epoller，管理epoll，相当于多路事件分发器的epoll实现
 * epoll的使用：
 * epoll_create()
 * epoll_ctl() add/mod/del
 * epoll_wait()
 */
class EpollPoller : public Poller {
public:	
	EpollPoller(EventLoop* loop);
	~EpollPoller() override; //!!override

	//基类纯虚函数的实现
	TimeStamp poll(int timeoutMs, ChannelList* activeChannel) override;
	void updateChannel(Channel* channel) override;
	void removeChannel(Channel* channel) override;
private:
	typedef std::vector<struct epoll_event> EventList; //vector not array, dynamicly expend
	static const int kInitEventListSize = 16;
	int m_epfd; //epoll例程文件描述符
	EventList m_events; //epoll返回的就绪事件数组

	void fillActiveChannels(int numEvents, ChannelList* activeChannels); //填写活跃Channels
	void update(int operation, Channel* channel); //更新Channels
};

#endif // !__EPOLLER_H_