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

const int kNew = -1; //Channel的成员m_index初始化为-1表示为注册到Poller
const int kAdded = 1; //Channel已经注册到Poller
const int kDeleted = 2; //Channel已经从Poller中注销了

//构造函数，创建epoll，初始化成员
EpollPoller::EpollPoller(EventLoop* loop) 
	: Poller(loop), 
	  m_epfd(::epoll_create1(EPOLL_CLOEXEC)),
	  m_events(kInitEventListSize) {
	if (m_epfd < 0) {
		LOG_FATAL("epoll_create error: %d\n", errno);
	}
}

//析构函数，释放资源，关闭文件
EpollPoller::~EpollPoller() {
	::close(m_epfd);
}

/**
 * 启动epoll_wait()，开始监视，由EventLoop调用
 * ChannelList* activeChannel由EventLoop创建并传递给Poller，当有就绪事件时Poller将就绪事件写入activeChannel
 * epoll_wait只调用一次，没有while，应该由EventLoop进行循环
 */
TimeStamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannel) {
	//实际上应该是LOG_DEBUG
	LOG_INFO("func %s: fd toatl = %u\n", __FUNCTION__, static_cast<unsigned int>(m_channelmap.size()));

	int numEvents = ::epoll_wait(m_epfd, &*m_events.begin(), 
		static_cast<int>(m_events.size()), timeoutMs);
	int saveErrno = errno;

	TimeStamp now(TimeStamp::now());

	if (numEvents > 0) {
		LOG_INFO("%d events happened\n", numEvents);
		fillActiveChannels(numEvents, activeChannel);

		//就绪事件处理后需要更新ChannelList(扩容)
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

//将就绪事件填写到活跃ChannelList
void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) {
	for (int i = 0; i < numEvents; ++i) {
		Channel* channel = static_cast<Channel*>(m_events[i].data.ptr);
		channel->set_revents(m_events[i].events);
		activeChannels->push_back(channel);
	}
}

//更新参数channel在Poller中的状态(add/mod/del)
void EpollPoller::updateChannel(Channel* channel) {
	//获取channel的index判断其状态
	const int index = channel->index();
	LOG_INFO("func %s: fd=%d event=%d index=%d\n", 
		__FUNCTION__, channel->fd(), channel->events(), channel->index());

	//状态为kNew或kDeleted
	if (index == kNew || index == kDeleted) {		

		//状态为kNew需要注册到Poller，先加入到map然后调用私有函数update执行epoll_ctl()
		if (index == kNew) {
			int fd = channel->fd();
			m_channelmap[fd] = channel;
		}
		channel->set_index(kAdded); //状态更新
		update(EPOLL_CTL_ADD, channel);
	}

	//状态为KAdded
	else {
		//检查channel的事件类型，如果是kNoneEvent说明这个channel不用监视了，需要被注销
		int fd = channel->fd();
		if (channel->isNoneEvent()) {
			channel->set_index(kDeleted);
			update(EPOLL_CTL_DEL, channel);
		}

		//如果不是则说明channel确实已经注册到Poller了但是现在仍然要update说明要修改其关心的事件类型
		else {
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

//在Poller中移除指定的channel
void EpollPoller::removeChannel(Channel* channel) {
	//先从channelmap中移除参数channel
	int fd = channel->fd();
	m_channelmap.erase(fd);

	LOG_INFO("func %s: fd=%d\n", __FUNCTION__, fd);

	//再从epoll_ctl中真正删除
	int index = channel->index();
	if (index == kAdded) {
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew); //状态更新
}

//通过epoll_ctl更新事件
void EpollPoller::update(int operation, Channel* channel) {
	struct epoll_event event;
	::memset(&event, 0, sizeof(epoll_event));
	int fd = channel->fd();
	event.events = channel->events();
	event.data.ptr = channel;
	
	if (::epoll_ctl(m_epfd, operation, fd, &event) < 0) {
		//对epoll_ctl失败分等级
		if (operation == EPOLL_CTL_DEL) {
			LOG_ERROR("epoll_ctl delete error, errno=%d\n", errno);
		}
		else {
			LOG_FATAL("fatal error in epoll_ctl, errno=%d\n", errno);
		}
	}
}