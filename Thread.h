#ifndef __THREAD_H_
#define __THREAD_H_

#include <unistd.h>
#include <thread>
#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include "noncopyable.h"

/**
 * ���̵߳ķ�װ��ʹ��C++STL
 */
class Thread : public noncopyable {
public:
	typedef std::function<void()> ThreadFunc; //���������� void (*)(void)�������Ҫ������ʹ��std::bind

	explicit Thread(ThreadFunc, const std::string& name = std::string());
	~Thread();

	void start();
	bool started() const { return m_started; }
	void join();
	pid_t tid() const { return m_tid; }
	const std::string& name() const { return m_name; }
	static int numCreated() { return m_numCreated; }
private:
	void setDefaultName();

	//������Ϣ
	bool m_started;
	bool m_join;

	//������ָ������߳���
	std::shared_ptr<std::thread> m_thread;

	pid_t m_tid;
	ThreadFunc m_func; //�߳�������
	std::string m_name; //�̰߳󶨵��ַ���(���ڵ���)
	static std::atomic_int32_t m_numCreated; //�������߳�����
};


#endif
