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

//�¼�ѭ���࣬��Ҫ��������ģ�飺Chnnel, Poller(epoll�ĳ���)
class EventLoop : public noncopyable {
public:
	typedef std::function<void()> Functor;

	EventLoop();
	~EventLoop();

	void loop(); //�����¼�ѭ��
	void quit(); //�˳��¼�ѭ��

	TimeStamp pollReturnTime() const { return m_pollReturnTime; } //��ȡPoller���صľ����¼���ʱ����

	void runInLoop(Functor cb); //�ڵ�ǰloop��ִ��cb
	void queueInLoop(Functor cb); //�Ѳ���cb��������У�����loop���ڵ��߳�ִ��cb
	
	void wakeup(); //����subloop���ڵ��߳�

	//Channel����EventLoop���ô˽ӿ���Poller�и���Channel
	void updateChannel(Channel* channel); 
	void removeChanell(Channel* channel);
	bool hasChannel(Channel* channel);
	bool isInLoopThread() const { return m_threadID == CurrentThread::tid(); }

private:
	typedef std::vector<Channel*> ChannelList;

	void handleRead(); //����Poller���õĶ������¼�
	void doPendingFunctors(); //ִ�лص�

	//��������
	std::atomic_bool m_looping; //ԭ���ౣ֤������д��ͬ���ԣ�ͨ��CASʵ�֣���ʶ�Ƿ�ѭ��
	std::atomic_bool m_quit; //�����˳�

	std::atomic_bool m_callingPendingFunctor; //��ʶ��ǰ�߳��Ƿ�����Ҫִ�лص�����
	const pid_t m_threadID; //�������loop���̵߳�tid
	TimeStamp m_pollReturnTime; //Poller���صķ��������¼���ʱ����
	std::unique_ptr<Poller> m_poller; //��̬����Poller��

	//�̼߳�ͨ��
	int m_wakeupFd; //mainloopͨ��eventfd����subloop����eventHandle
	std::unique_ptr<Channel> m_wakeupChannel; //�������ָ��������m_wakeupFd

	ChannelList m_activeChannels;

	std::vector<Functor> m_pendingFunctors; //�ص��������󼯺�
	std::mutex m_mutex; //ͬ�����ƣ���֤vector������д���̰߳�ȫ
};

#endif // !__EVENTLOOP_H_
