#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

/**
 *             EventLoop
 * ChannelList           Poller
 * fd,event,revent,cb    ChannelMap<fd, channel*>
 */

const int kNew = -1; //Channel�ĳ�Աm_index��ʼ��Ϊ-1��ʾΪע�ᵽPoller
const int kAdded = 1; //Channel�Ѿ�ע�ᵽPoller
const int kDeleted = 2; //Channel�Ѿ���Poller��ע����

//���캯��������epoll����ʼ����Ա
EpollPoller::EpollPoller(EventLoop* loop) 
	: Poller(loop), 
	  m_epfd(::epoll_create1(EPOLL_CLOEXEC)),
	  m_events(kInitEventListSize) {
	if (m_epfd < 0) {
		LOG_FATAL("epoll_create error: %d\n", errno);
	}
}

//�����������ͷ���Դ���ر��ļ�
EpollPoller::~EpollPoller() {
	::close(m_epfd);
}

/**
 * ����epoll_wait()����ʼ���ӣ���EventLoop����
 * ChannelList* activeChannel��EventLoop���������ݸ�Poller�����о����¼�ʱPoller�������¼�д��activeChannel
 * epoll_waitֻ����һ�Σ�û��while��Ӧ����EventLoop����ѭ��
 */
TimeStamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannel) {
	//ʵ����Ӧ����LOG_DEBUG
	LOG_INFO("func %s: fd toatl = %u\n", __FUNCTION__, static_cast<unsigned int>(m_channelmap.size()));

	int numEvents = ::epoll_wait(m_epfd, &*m_events.begin(), 
		static_cast<int>(m_events.size()), timeoutMs);
	int saveErrno = errno;

	TimeStamp now(TimeStamp::now());

	if (numEvents > 0) {
		LOG_INFO("%d events happened\n", numEvents);
		fillActiveChannels(numEvents, activeChannel);

		//�����¼��������Ҫ����ChannelList(����)
		if (numEvents == m_events.size()) {
			m_events.resize(m_events.size() * 2);
		}
	}
	else if (numEvents == 0) {
		LOG_DEBUG("%s: timeout\n", __FUNCTION__);
	}
	else {
		if (saveErrno != EINTR) {
			errno = saveErrno;
			LOG_ERROR("%s err\n", __FUNCTION__);
		}
	}

	return now;
}

//�������¼���д����ԾChannelList
void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) {
	for (int i = 0; i < numEvents; ++i) {
		Channel* channel = static_cast<Channel*>(m_events[i].data.ptr);
		channel->set_revents(m_events[i].events);
		activeChannels->push_back(channel);
	}
}

//���²���channel��Poller�е�״̬(add/mod/del)
void EpollPoller::updateChannel(Channel* channel) {
	//��ȡchannel��index�ж���״̬
	const int index = channel->index();
	LOG_INFO("func %s: fd=%d event=%d index=%d\n", 
		__FUNCTION__, channel->fd(), channel->events(), channel->index());

	//״̬ΪkNew��kDeleted
	if (index == kNew || index == kDeleted) {		

		//״̬ΪkNew��Ҫע�ᵽPoller���ȼ��뵽mapȻ�����˽�к���updateִ��epoll_ctl()
		if (index == kNew) {
			int fd = channel->fd();
			m_channelmap[fd] = channel;
		}
		channel->set_index(kAdded); //״̬����
		update(EPOLL_CTL_ADD, channel);
	}

	//״̬ΪKAdded
	else {
		//���channel���¼����ͣ������kNoneEvent˵�����channel���ü����ˣ���Ҫ��ע��
		int fd = channel->fd();
		if (channel->isNoneEvent()) {
			channel->set_index(kDeleted);
			update(EPOLL_CTL_DEL, channel);
		}

		//���������˵��channelȷʵ�Ѿ�ע�ᵽPoller�˵���������ȻҪupdate˵��Ҫ�޸�����ĵ��¼�����
		else {
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

//��Poller���Ƴ�ָ����channel
void EpollPoller::removeChannel(Channel* channel) {
	//�ȴ�channelmap���Ƴ�����channel
	int fd = channel->fd();
	m_channelmap.erase(fd);

	LOG_INFO("func %s: fd=%d\n", __FUNCTION__, fd);

	//�ٴ�epoll_ctl������ɾ��
	int index = channel->index();
	if (index == kAdded) {
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew); //״̬����
}

//ͨ��epoll_ctl�����¼�
void EpollPoller::update(int operation, Channel* channel) {
	struct epoll_event event;
	::memset(&event, 0, sizeof(epoll_event));
	int fd = channel->fd();
	event.events = channel->events();
	event.data.ptr = channel;
	
	if (::epoll_ctl(m_epfd, operation, fd, &event) < 0) {
		//��epoll_ctlʧ�ֵܷȼ�
		if (operation == EPOLL_CTL_DEL) {
			LOG_ERROR("epoll_ctl delete error, errno=%d\n", errno);
		}
		else {
			LOG_FATAL("fatal error in epoll_ctl, errno=%d\n", errno);
		}
	}
}