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
	void start(const ThreadInitCallback& cb = ThreadInitCallback()); //Ĭ�Ϲ���

	//��������ڶ��̻߳�����Ĭ������ѯ�ķ�ʽ����channel(�����ӵ�socket)��subloop
	EventLoop* getNextLoop();

	//��ȡ���е�loop����m_loops������һ�ݣ��ܲ����� const std::vector<EventLoop*>& ??
	std::vector<EventLoop*> getAllLoops();

	bool started() const { return m_started; }
	const std::string& name() const { return m_name; }
private:
	EventLoop* m_baseLoop; //Ĭ�ϴ�����loop(mainloop)
	std::string m_name;
	int m_numThreads; //�����߳�����
	int m_next;
	bool m_started;
	std::vector<std::unique_ptr<EventLoopThread>> m_threads; //���й����߳�
	std::vector<EventLoop*> m_loops; //���е��¼�ѭ��
};

#endif // !__EVENTLOOPTHREADPOOL_H
