#ifndef __POLLER_H_
#define __POLLER_H_

#include <vector>
#include <unordered_map>
#include "TimeStamp.h"
#include "noncopyable.h"

class EventLoop;
class Channel;

//抽象基类，epoll和poll的抽象封装
class Poller : public noncopyable {
public:
	typedef std::vector<Channel*> ChannelList; //Poller管理的Channel集合
	Poller(EventLoop* loop);
	virtual ~Poller(); //虚析构函数

	//纯虚函数，提供统一的IO多路复用的接口
	virtual TimeStamp poll(int timeoutMs, ChannelList* activeChannel) = 0;
	virtual void updateChannel(Channel* channel) = 0;
	virtual void removeChannel(Channel* channel) = 0;
	virtual bool hasChannel(Channel* channel) const; //参数Channel是否在这个Poller中

	//EventLoop调用，返回默认的Poller，也就是默认具体的IO多路复用的实现
	static Poller* newDefaultPoller(EventLoop* loop);

protected:
	//map映射的是fd和其对应的channel，key：fd，value：所属的Channel
	typedef std::unordered_map<int, Channel*> ChannelMap; //map not vector, easy to find
	ChannelMap m_channelmap;
private:
	EventLoop* m_loop; //此Poller属于哪一个事件循环
};

#endif // !__POLLER_H_
