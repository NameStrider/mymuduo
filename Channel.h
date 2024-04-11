#ifndef __CHANNEL_H_
#define __CHANNEL_H_

#include <functional>
#include <memory>
#include "noncopyable.h"
#include "TimeStamp.h"

class EventLoop;

//IO通道，管理一个fd及其对应的事件类型，还绑定了Poller返回的就绪事件及其处理方法(回调函数)
class Channel : public noncopyable {
public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(TimeStamp)> ReadEventCallback;

	Channel(EventLoop* loop, int fd);
	~Channel();

	//处理Poller返回的就绪事件，通过指定的回调方法处理
	void handleEvent(TimeStamp timestamp); //不是传引用？

	//接口函数，设置回调方法，注意 std::move() 的作用和意义
	void setReadCallback(ReadEventCallback cb) { m_readCallback = std::move(cb); }
	void setWriteCallback(EventCallback cb) { m_writeCallback = std::move(cb); }
	void setCloseCallback(EventCallback cb) { m_closeCallback = std::move(cb); }
	void setErrorCallback(EventCallback cb) { m_errorCallback = std::move(cb); }

	//防止Channel被手动remove掉的时候，回调函数对象还在执行
	void tie(const std::shared_ptr<void>&);

	//读写文件描述符及其关心的事件类型
	int fd() const { return m_fd; }
	int events() const { return m_events; }
	void set_revents(int revt) { m_revents = revt; } //由Poller来设置

	//设置fd对应的事件状态以便Poller正确监视
	void enableReading() { m_events |= kReadEvent; update(); }
	void disableReading() { m_events &= ~kReadEvent; update(); }
	void enableWriting() { m_events |= kWriteEvent; update(); }
	void disableWriting() { m_events &= ~kWriteEvent; update(); }
	void disableAll() { m_events = kNoneEvent; update(); }

	//返回此时fd关心的事件类型
	bool isNoneEvent() const{ return m_events == kNoneEvent; }
	bool isReadingEvent() const{ return m_events & kReadEvent; }
	bool isWritingEvent() const{ return m_events & kWriteEvent; }

	//
	int index() const { return m_index; }
	int set_index(int idx) { m_index = idx; }

	//返回此Channel属于哪一个EventLoop
	EventLoop* ownerLoop() { return m_loop; }
	void remove();
private:
	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;
private:
	const int m_fd; //一个Channel绑定一个fd
	int m_events; //fd对应的事件类型
	int m_revents; //Poller返回的就绪事件
	int m_index; //Poller使用
	EventLoop* m_loop; //属于哪个事件循环

	std::weak_ptr<void> m_tie;
	bool m_tied;

	void update();
	void handleEventWithGuad(TimeStamp recvtime);

	ReadEventCallback m_readCallback; //读事件回调方法
	EventCallback m_writeCallback; //写事件回调方法
	EventCallback m_closeCallback; //连接关闭回调方法
	EventCallback m_errorCallback; //错误事件回调方法
};

#endif // !__CHANNEL_H_
