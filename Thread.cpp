#include <semaphore.h>
#include "Thread.h"
#include "CurrentThread.h"

std::atomic_int32_t Thread::m_numCreated(0);

Thread::Thread(ThreadFunc func, const std::string& name) 
	: m_started(false),
	  m_join(false),
	  m_tid(0),
	  m_func(std::move(func)),
	  m_name(name) 
{
	setDefaultName();
}

Thread::~Thread() {
	if (m_started && !m_join) {
		m_thread->detach(); //std::thread�ṩ�� pthread_detach() �Ľӿ� 
	}
}

void Thread::setDefaultName() {
	int num = ++m_numCreated;
	if (m_name.empty()) {
		char buffer[32] = {0};
		snprintf(buffer, sizeof(buffer), "Thread%d", num);
		m_name = buffer;
	}
}

void Thread::start() {
	m_started = true;

	sem_t sem;
	sem_init(&sem, false, 0);

	//�������߳�(�����߳�)
	m_thread = std::shared_ptr<std::thread>(new std::thread([&]() {
		m_tid = CurrentThread::tid();
		sem_post(&sem);
		m_func(); //ִ��������
	}));

	//�ȴ������߳�������tid
	sem_wait(&sem);
}

void Thread::join() {
	m_join = true;
	m_thread->join();
}