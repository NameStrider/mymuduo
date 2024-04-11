#include <memory>
#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name) 
	: m_loop(NULL),
	  m_mutex(),
	  m_cond(),
	  m_exiting(false),
	  m_callback(cb),
	  m_thread(std::bind(&EventLoopThread::threadFunc, this), name) //Ϊʲô��Ҫthis
{

}

EventLoopThread::~EventLoopThread() {
	m_exiting = true;

	if (m_loop) {
		m_loop->quit();
		m_thread.join();
	}
}

//�������߳�
EventLoop* EventLoopThread::startLoop() {
	//1.�������������߳�
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

	return loop; //���߳��е�EventLoop
}

//��������洴�����̵߳�����ִ�е�������
void EventLoopThread::threadFunc() {
	//1.����һ��������EventLoop�������洴�������߳�һһ��Ӧ�����˲��������� one loop per thread
	EventLoop loop;

	//2.�����洴����loop�󶨵��߳�
	if (m_callback) {
		m_callback(&loop);
	}

	//3.����m_loop��ͨ����������֪ͨ(��������̵߳��߳�)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_loop = &loop;
		m_cond.notify_one();
	}

	//4.���߳��������¼�ѭ��
	loop.loop(); //EventLoop.loop() => Poller=>poll() => epoll_wait()

	//5.�¼�ѭ���˳�����������
	std::unique_lock<std::mutex> lock(m_mutex);
	m_loop = NULL;
}