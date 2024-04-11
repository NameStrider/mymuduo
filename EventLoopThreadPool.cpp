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

//不需要析构 std::vector<EventLoop*> m_loops; 因为 EventLoop* 指向的内存是栈段中的局部变量，会自动释放
EventLoopThreadPool::~EventLoopThreadPool() {

}

//启动EventLoopThreadPool
void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
	//1.设置m_started为true，表示EventLoopThreadPool开始启动
	m_started = true;

	//2.创建EventLoopThread
	for (int i = 0; i < m_numThreads; ++i) {

		//初始化工作线程的name
		char buffer[m_name.size() + 32];
		snprintf(buffer, sizeof(buffer), "%s%d", m_name.c_str(), i);

		//创建EventLoopThread类，此时还没有创建线程
		EventLoopThread* t = new EventLoopThread(cb, buffer);

		//将新创建的EventLoopThread类用智能指针封装后加入集合
		m_threads.push_back(std::unique_ptr<EventLoopThread>(t));

		//将新创建的EventLoopThread类中的线程执行的loop加入集合，此时创建了工作线程
		m_loops.push_back(t->startLoop());
	}

	//3.特殊情况，只有baseloop一个线程
	if (m_numThreads == 0 && cb) {
		cb(m_baseLoop);
	}
}

//以轮询的方式分配channel(新连接的socket)给subloop
EventLoop* EventLoopThreadPool::getNextLoop() {
	EventLoop* loop = m_baseLoop;
	
	//round robin 获取下一个EventLoop
	if (!m_loops.empty()) {
		loop = m_loops[m_next];
		++m_next; //不能越界
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