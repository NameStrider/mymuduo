#ifndef __EVENTLOOP_H_
#define __EVENTLOOP_H_

#include <mutex>
#include <atomic>
#include <vector>
#include <functional>
#include <memory>

#include "TimeStamp.h"
#include "CurrentThread.h"
#include "noncopyable.h"

class Channel;
class Poller;

//事件循环类，主要包括两个模块：Chnnel, Poller(epoll的抽象)
class EventLoop : public noncopyable {
public:
	typedef std::function<void()> Functor;

	EventLoop();
	~EventLoop();

	void loop(); //开启事件循环
	void quit(); //退出事件循环

	TimeStamp pollReturnTime() const { return m_pollReturnTime; } //读取Poller返回的就绪事件的时间类

	void runInLoop(Functor cb); //在当前loop中执行cb
	void queueInLoop(Functor cb); //把参数cb放入队列中，唤醒loop所在的线程执行cb
	
	void wakeup(); //唤醒subloop所在的线程

	//Channel申请EventLoop调用此接口在Poller中更新Channel
	void updateChannel(Channel* channel); 
	void removeChanell(Channel* channel);
	bool hasChannel(Channel* channel);
	bool isInLoopThread() const { return m_threadID == CurrentThread::tid(); }

private:
	typedef std::vector<Channel*> ChannelList;

	void handleRead(); //处理Poller设置的读就绪事件
	void doPendingFunctors(); //执行回调

	//控制属性
	std::atomic_bool m_looping; //原子类保证并发读写的同步性，通过CAS实现，标识是否循环
	std::atomic_bool m_quit; //优雅退出

	std::atomic_bool m_callingPendingFunctor; //标识当前线程是否有需要执行回调操作
	const pid_t m_threadID; //创建这个loop的线程的tid
	TimeStamp m_pollReturnTime; //Poller返回的发生就绪事件的时间类
	std::unique_ptr<Poller> m_poller; //动态管理Poller类

	//线程间通信
	int m_wakeupFd; //mainloop通过eventfd唤醒subloop进行eventHandle
	std::unique_ptr<Channel> m_wakeupChannel; //这个智能指针管理的是m_wakeupFd

	ChannelList m_activeChannels;

	std::vector<Functor> m_pendingFunctors; //回调函数对象集合
	std::mutex m_mutex; //同步机制，保证vector并发读写的线程安全
};

#endif // !__EVENTLOOP_H_
