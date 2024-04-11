#ifndef __CHANNEL_H_
#define __CHANNEL_H_

#include <functional>
#include <memory>
#include "noncopyable.h"
#include "TimeStamp.h"

class EventLoop;

//IOͨ��������һ��fd�����Ӧ���¼����ͣ�������Poller���صľ����¼����䴦����(�ص�����)
class Channel : public noncopyable {
public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(TimeStamp)> ReadEventCallback;

	Channel(EventLoop* loop, int fd);
	~Channel();

	//����Poller���صľ����¼���ͨ��ָ���Ļص���������
	void handleEvent(TimeStamp timestamp); //���Ǵ����ã�

	//�ӿں��������ûص�������ע�� std::move() �����ú�����
	void setReadCallback(ReadEventCallback cb) { m_readCallback = std::move(cb); }
	void setWriteCallback(EventCallback cb) { m_writeCallback = std::move(cb); }
	void setCloseCallback(EventCallback cb) { m_closeCallback = std::move(cb); }
	void setErrorCallback(EventCallback cb) { m_errorCallback = std::move(cb); }

	//��ֹChannel���ֶ�remove����ʱ�򣬻ص�����������ִ��
	void tie(const std::shared_ptr<void>&);

	//��д�ļ�������������ĵ��¼�����
	int fd() const { return m_fd; }
	int events() const { return m_events; }
	void set_revents(int revt) { m_revents = revt; } //��Poller������

	//����fd��Ӧ���¼�״̬�Ա�Poller��ȷ����
	void enableReading() { m_events |= kReadEvent; update(); }
	void disableReading() { m_events &= ~kReadEvent; update(); }
	void enableWriting() { m_events |= kWriteEvent; update(); }
	void disableWriting() { m_events &= ~kWriteEvent; update(); }
	void disableAll() { m_events = kNoneEvent; update(); }

	//���ش�ʱfd���ĵ��¼�����
	bool isNoneEvent() const{ return m_events == kNoneEvent; }
	bool isReadingEvent() const{ return m_events & kReadEvent; }
	bool isWritingEvent() const{ return m_events & kWriteEvent; }

	//
	int index() const { return m_index; }
	int set_index(int idx) { m_index = idx; }

	//���ش�Channel������һ��EventLoop
	EventLoop* ownerLoop() { return m_loop; }
	void remove();
private:
	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;
private:
	const int m_fd; //һ��Channel��һ��fd
	int m_events; //fd��Ӧ���¼�����
	int m_revents; //Poller���صľ����¼�
	int m_index; //Pollerʹ��
	EventLoop* m_loop; //�����ĸ��¼�ѭ��

	std::weak_ptr<void> m_tie;
	bool m_tied;

	void update();
	void handleEventWithGuad(TimeStamp recvtime);

	ReadEventCallback m_readCallback; //���¼��ص�����
	EventCallback m_writeCallback; //д�¼��ص�����
	EventCallback m_closeCallback; //���ӹرջص�����
	EventCallback m_errorCallback; //�����¼��ص�����
};

#endif // !__CHANNEL_H_
