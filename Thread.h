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
 * 对线程的封装，使用C++STL
 */
class Thread : public noncopyable {
public:
	typedef std::function<void()> ThreadFunc; //函数类型是 void (*)(void)，如果需要参数就使用std::bind

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

	//属性信息
	bool m_started;
	bool m_join;

	//用智能指针管理线程类
	std::shared_ptr<std::thread> m_thread;

	pid_t m_tid;
	ThreadFunc m_func; //线程主函数
	std::string m_name; //线程绑定的字符串(用于调试)
	static std::atomic_int32_t m_numCreated; //创建的线程数量
};


#endif
