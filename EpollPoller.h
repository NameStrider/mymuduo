#ifndef __EPOLLER_H_
#define __EPOLLER_H_

#include <vector>
#include <sys/epoll.h>
#include "Poller.h"

/**
 * Poller��������Epoller������epoll���൱�ڶ�·�¼��ַ�����epollʵ��
 * epoll��ʹ�ã�
 * epoll_create()
 * epoll_ctl() add/mod/del
 * epoll_wait()
 */
class EpollPoller : public Poller {
public:	
	EpollPoller(EventLoop* loop);
	~EpollPoller() override; //!!override

	//���ി�麯����ʵ��
	TimeStamp poll(int timeoutMs, ChannelList* activeChannel) override;
	void updateChannel(Channel* channel) override;
	void removeChannel(Channel* channel) override;
private:
	typedef std::vector<struct epoll_event> EventList; //vector not array, dynamicly expend
	static const int kInitEventListSize = 16;
	int m_epfd; //epoll�����ļ�������
	EventList m_events; //epoll���صľ����¼�����

	void fillActiveChannels(int numEvents, ChannelList* activeChannels); //��д��ԾChannels
	void update(int operation, Channel* channel); //����Channels
};

#endif // !__EPOLLER_H_