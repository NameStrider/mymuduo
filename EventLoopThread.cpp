#include <memory>
#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name) 
	: m_loop(NULL),
	  m_mutex(),
	  m_cond(),
	  m_exiting(false),
	  m_callback(cb),
	  m_thread(std::bind(&EventLoopThread::threadFunc, this), name) //为什么需要this
{

}

EventLoopThread::~EventLoopThread() {
	m_exiting = true;

	if (m_loop) {
		m_loop->quit();
		m_thread.join();
	}
}

//创建新线程
EventLoop* EventLoopThread::startLoop() {
	//1.创建并启动新线程
	m_thread.start();

	//2.
	EventLoop* loop = NULL;
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		while (m_loop == NULL) {
			m_cond.wait(lock);
		}
		loop = m_loop;
	}

	return loop; //新线程中的EventLoop
}

//这就是上面创建的线程的真正执行的任务函数
void EventLoopThread::threadFunc() {
	//1.创建一个独立的EventLoop，与上面创建的新线程一一对应，至此才是真正的 one loop per thread
	EventLoop loop;

	//2.将上面创建的loop绑定到线程
	if (m_callback) {
		m_callback(&loop);
	}

	//3.更新m_loop并通过条件变量通知(创建这个线程的线程)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_loop = &loop;
		m_cond.notify_one();
	}

	//4.在线程中启动事件循环
	loop.loop(); //EventLoop.loop() => Poller=>poll() => epoll_wait()

	//5.事件循环退出，变量更新
	std::unique_lock<std::mutex> lock(m_mutex);
	m_loop = NULL;
}