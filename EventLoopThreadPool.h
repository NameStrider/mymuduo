#ifndef __EVENTLOOPTHREADPOOL_H
#define __EVENTLOOPTHREADPOOL_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "noncopyable.h"
#include "EventLoopThread.h"

class EventLoop;

class EventLoopThreadPool : public noncopyable {
public:
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
	~EventLoopThreadPool();

	void setThreadNum(int num) { m_numThreads = num; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback()); //默认构造

	//如果工作在多线程环境中默认以轮询的方式分配channel(新连接的socket)给subloop
	EventLoop* getNextLoop();

	//获取所有的loop，将m_loops拷贝了一份，能不能是 const std::vector<EventLoop*>& ??
	std::vector<EventLoop*> getAllLoops();

	bool started() const { return m_started; }
	const std::string& name() const { return m_name; }
private:
	EventLoop* m_baseLoop; //默认创建的loop(mainloop)
	std::string m_name;
	int m_numThreads; //工作线程数量
	int m_next;
	bool m_started;
	std::vector<std::unique_ptr<EventLoopThread>> m_threads; //所有工作线程
	std::vector<EventLoop*> m_loops; //所有的事件循环
};

#endif // !__EVENTLOOPTHREADPOOL_H
