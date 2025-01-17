#ifndef __EVENTLOOPTHREAD_H_
#define __EVENTLOOPTHREAD_H_

#include <mutex>
#include <string>
#include <condition_variable>
#include <functional>
#include "Thread.h"
#include "noncopyable.h"

class EventLoop;

//绑定了事件循环的线程类
class EventLoopThread : public noncopyable {
public:
	typedef std::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
		const std::string& name = std::string());
	~EventLoopThread();

	EventLoop* startLoop();
private:
	void threadFunc();

	EventLoop* m_loop;
	bool m_exiting;
	Thread m_thread;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	ThreadInitCallback m_callback;
};

#endif // !__EVENTLOOPTHREAD_H_
