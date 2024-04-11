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
 *	          eventfd       û��ͨ���̰߳�ȫ�Ķ���ʵ��������-������ģ��
 * subloop_1           subloop_2
 */

//��ֹһ���̴߳������loop
thread_local EventLoop* t_loopInThisThread = NULL;

//Ĭ�ϵ�Poller��ʱʱ��
const int kpollTimeoutMs = 10000;

//����eventfd������notify����subReactor
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

	//��֤ one loop per thread
	if (t_loopInThisThread) {
		LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, m_threadID);
	}
	else {
		t_loopInThisThread = this;
	}

	//���� m_wakeupFd �Ķ��ص�����
	m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this)); //std::bind ��ʹ��
	
	//���ú�ÿ���̶߳������m_wakeupChannel��EPOLLIN�������¼�(����->epoll->����)
	m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop() {
	//1.���ټ���eventfd
	m_wakeupChannel->disableAll();

	//2.��Poller���Ƴ�channel
	m_wakeupChannel->remove();

	//3.�ر��ļ�
	::close(m_wakeupFd);

	//4.���ֲ߳̾���������ΪNULL
	t_loopInThisThread = NULL;
}

//�����¼�ѭ��
void EventLoop::loop() {
	m_looping = true;
	m_quit = false;

	LOG_INFO("EventLoop %p start looping\n", this);

	while (!m_quit) {
		m_activeChannels.clear(); //m_activeChannels�Ǹ��õģ�ÿ�ζ�Ҫ����
		m_pollReturnTime = m_poller->poll(kpollTimeoutMs, &m_activeChannels);

		//Poller���ؼ����¼������еľ����¼���֪ͨEventLoop��EventLoop����Channel�еĴ�����
		for (Channel* channel : m_activeChannels) {
			channel->handleEvent(m_pollReturnTime);
		}

		//mainloopע��Ļص��������󣬵�mainloop wakeup subloop��subloopִ������Ļص�
		doPendingFunctors();
	}

	LOG_INFO("EventLoop %p end looping\n", this);
	m_looping = false;
}

/**
 * �˳��¼�ѭ�������������
 * 1.���߳����Լ���subloop�е��ã�������m_quit����Ϊtrue��ѭ���˳�
 * 2.���߳̽����߳�mainloop�е�m_quit����Ϊtrue����ʱmainloop���ܴ�������״̬�޷��˳�������Ҫͨ��eventfd����
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

//�ڵ�ǰloopִ�лص�
void EventLoop::runInLoop(Functor cb) {
	if (isInLoopThread()) { //�ڵ�ǰloop�߳���ִ�лص�
		cb();
	}
	else { //���ڵ�ǰloop�̣߳���Ҫ����loop�����߳�ִ��cb
		queueInLoop(cb);
	}
}

//��cb������У�����loop�����߳�ִ�лص�
void EventLoop::queueInLoop(Functor cb) {
	//1.��cb������У�������Ϊ�̵߳Ĺ�����Դ��Ҫ����
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_pendingFunctors.emplace_back(cb);
	}
	
	//2.������Ҫִ�������cb��loop���ڵ��߳�
	if (!isInLoopThread() || m_callingPendingFunctor) { //!!m_callingPendingFunctor��Ϊ�жϵ�����

		//ִ��doPendingFunctors��loop���ڵ��߳̿�������ִ�лص������Ǵ�ʱ���������µĻص�
		//���߳̾Ͳ���������Ӧ��ִ���µĻص�
		wakeup(); 
	}
}

/**
 * ����loop�����̣߳�ͨ����eventfdд�����ݣ����м����� m_wakeupChannel ��loop���ڵ��̶߳��ᱻ���ѣ�
 * Poller���ض������¼�
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
	std::vector<Functor> Functors; //�ֲ�����������
	m_callingPendingFunctor = true;

	{
		std::unique_lock<std::mutex> lock(m_mutex);
		Functors.swap(m_pendingFunctors);
	}

	for (const Functor& functor : Functors) {
		functor(); //ִ�е�ǰloop��Ҫ�Ļص�
	}

	m_callingPendingFunctor = false;
}