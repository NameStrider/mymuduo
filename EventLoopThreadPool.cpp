#include <memory>
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg)
	: m_baseLoop(baseLoop),
	  m_name(nameArg),
	  m_numThreads(0),
	  m_started(false),
	  m_next(0)
{
}

//����Ҫ���� std::vector<EventLoop*> m_loops; ��Ϊ EventLoop* ָ����ڴ���ջ���еľֲ����������Զ��ͷ�
EventLoopThreadPool::~EventLoopThreadPool() {

}

//����EventLoopThreadPool
void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
	//1.����m_startedΪtrue����ʾEventLoopThreadPool��ʼ����
	m_started = true;

	//2.����EventLoopThread
	for (int i = 0; i < m_numThreads; ++i) {

		//��ʼ�������̵߳�name
		char buffer[m_name.size() + 32];
		snprintf(buffer, sizeof(buffer), "%s%d", m_name.c_str(), i);

		//����EventLoopThread�࣬��ʱ��û�д����߳�
		EventLoopThread* t = new EventLoopThread(cb, buffer);

		//���´�����EventLoopThread��������ָ���װ����뼯��
		m_threads.push_back(std::unique_ptr<EventLoopThread>(t));

		//���´�����EventLoopThread���е��߳�ִ�е�loop���뼯�ϣ���ʱ�����˹����߳�
		m_loops.push_back(t->startLoop());
	}

	//3.���������ֻ��baseloopһ���߳�
	if (m_numThreads == 0 && cb) {
		cb(m_baseLoop);
	}
}

//����ѯ�ķ�ʽ����channel(�����ӵ�socket)��subloop
EventLoop* EventLoopThreadPool::getNextLoop() {
	EventLoop* loop = m_baseLoop;
	
	//round robin ��ȡ��һ��EventLoop
	if (!m_loops.empty()) {
		loop = m_loops[m_next];
		++m_next; //����Խ��
		if (m_next >= m_loops.size()) {
			m_next = 0;
		}
	}

	return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
	if (m_loops.empty()) {
		return std::vector<EventLoop*>(1, m_baseLoop);
	}

	return m_loops;
}