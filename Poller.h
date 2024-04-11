#ifndef __POLLER_H_
#define __POLLER_H_

#include <vector>
#include <unordered_map>
#include "TimeStamp.h"
#include "noncopyable.h"

class EventLoop;
class Channel;

//������࣬epoll��poll�ĳ����װ
class Poller : public noncopyable {
public:
	typedef std::vector<Channel*> ChannelList; //Poller�����Channel����
	Poller(EventLoop* loop);
	virtual ~Poller(); //����������

	//���麯�����ṩͳһ��IO��·���õĽӿ�
	virtual TimeStamp poll(int timeoutMs, ChannelList* activeChannel) = 0;
	virtual void updateChannel(Channel* channel) = 0;
	virtual void removeChannel(Channel* channel) = 0;
	virtual bool hasChannel(Channel* channel) const; //����Channel�Ƿ������Poller��

	//EventLoop���ã�����Ĭ�ϵ�Poller��Ҳ����Ĭ�Ͼ����IO��·���õ�ʵ��
	static Poller* newDefaultPoller(EventLoop* loop);

protected:
	//mapӳ�����fd�����Ӧ��channel��key��fd��value��������Channel
	typedef std::unordered_map<int, Channel*> ChannelMap; //map not vector, easy to find
	ChannelMap m_channelmap;
private:
	EventLoop* m_loop; //��Poller������һ���¼�ѭ��
};

#endif // !__POLLER_H_
