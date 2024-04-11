#include <errno.h>
#include <memory>
#include <unistd.h>
#include <functional>
#include <sys/eventfd.h>
#include "Poller.h"
#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"

/**
 *            mainloop
 *	          eventfd       没有通过线程安全的队列实现生产者-消费者模型
 * subloop_1           subloop_2
 */

//防止一个线程创建多个loop
thread_local EventLoop* t_loopInThisThread = NULL;

//默认的Poller超时时间
const int kpollTimeoutMs = 10000;

//创建eventfd，用于notify唤醒subReactor
int createEventfd() {
	int evtfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (evtfd < 0) {
		LOG_FATAL("eventfd create error, errno=%d\n", errno);
	}

	return evtfd;
}

EventLoop::EventLoop() 
	: m_looping(false),
	  m_quit(false),
	  m_callingPendingFunctor(false),
	  m_threadID(CurrentThread::tid()),
	  m_poller(Poller::newDefaultPoller(this)),
	  m_wakeupFd(createEventfd()),
	  m_wakeupChannel(new Channel(this, m_wakeupFd))
{
	LOG_DEBUG("EventLoop created %p in thread %d\n", this, m_threadID);

	//保证 one loop per thread
	if (t_loopInThisThread) {
		LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, m_threadID);
	}
	else {
		t_loopInThisThread = this;
	}

	//设置 m_wakeupFd 的读回调方法
	m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this)); //std::bind 的使用
	
	//设置后每个线程都会监视m_wakeupChannel的EPOLLIN读就绪事件(阻塞->epoll->唤醒)
	m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop() {
	//1.不再监视eventfd
	m_wakeupChannel->disableAll();

	//2.从Poller中移除channel
	m_wakeupChannel->remove();

	//3.关闭文件
	::close(m_wakeupFd);

	//4.将线程局部变量设置为NULL
	t_loopInThisThread = NULL;
}

//开启事件循环
void EventLoop::loop() {
	m_looping = true;
	m_quit = false;

	LOG_INFO("EventLoop %p start looping\n", this);

	while (!m_quit) {
		m_activeChannels.clear(); //m_activeChannels是复用的，每次都要清零
		m_pollReturnTime = m_poller->poll(kpollTimeoutMs, &m_activeChannels);

		//Poller返回监视事件集合中的就绪事件并通知EventLoop，EventLoop调用Channel中的处理方法
		for (Channel* channel : m_activeChannels) {
			channel->handleEvent(m_pollReturnTime);
		}

		//mainloop注册的回调函数对象，当mainloop wakeup subloop后，subloop执行下面的回调
		doPendingFunctors();
	}

	LOG_INFO("EventLoop %p end looping\n", this);
	m_looping = false;
}

/**
 * 退出事件循环，两种情况：
 * 1.子线程在自己的subloop中调用，将变量m_quit设置为true后，循环退出
 * 2.子线程将主线程mainloop中的m_quit设置为true，此时mainloop可能处于阻塞状态无法退出，所以要通过eventfd唤醒
 */
void EventLoop::quit() {
	m_quit = true;
	if (!isInLoopThread()) {
		wakeup();
	}
}

void EventLoop::handleRead() {
	uint64_t one = 1;
	int n = ::read(m_wakeupFd, &one, sizeof(one));
	if (n != sizeof(one)) {
		LOG_ERROR("EventLoop::handleRead read %d bytes instead of 8\n", n);
	}
}

//在当前loop执行回调
void EventLoop::runInLoop(Functor cb) {
	if (isInLoopThread()) { //在当前loop线程中执行回调
		cb();
	}
	else { //不在当前loop线程，需要唤醒loop所在线程执行cb
		queueInLoop(cb);
	}
}

//将cb放入队列，唤醒loop所在线程执行回调
void EventLoop::queueInLoop(Functor cb) {
	//1.将cb加入队列，队列作为线程的共享资源需要加锁
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_pendingFunctors.emplace_back(cb);
	}
	
	//2.唤醒需要执行上面的cb的loop所在的线程
	if (!isInLoopThread() || m_callingPendingFunctor) { //!!m_callingPendingFunctor作为判断的意义

		//执行doPendingFunctors的loop所在的线程可能正在执行回调，但是此时可能又有新的回调
		//该线程就不该阻塞，应该执行新的回调
		wakeup(); 
	}
}

/**
 * 唤醒loop所在线程，通过向eventfd写入数据，所有监视了 m_wakeupChannel 的loop所在的线程都会被唤醒，
 * Poller返回读就绪事件
 */
void EventLoop::wakeup() {
	uint64_t one = 1;
	int n = ::write(m_wakeupFd, &one, sizeof(one));
	if (n != sizeof(one)) {
		LOG_ERROR("EventLoop::wakeup write %d bytes instead of 8\n", n);
	}
}

void EventLoop::updateChannel(Channel* channel) {
	m_poller->updateChannel(channel);
}

void EventLoop::removeChanell(Channel* channel) {
	m_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
	return m_poller->hasChannel(channel); //!return!
}

void EventLoop::doPendingFunctors() {
	std::vector<Functor> Functors; //局部变量的意义
	m_callingPendingFunctor = true;

	{
		std::unique_lock<std::mutex> lock(m_mutex);
		Functors.swap(m_pendingFunctors);
	}

	for (const Functor& functor : Functors) {
		functor(); //执行当前loop需要的回调
	}

	m_callingPendingFunctor = false;
}