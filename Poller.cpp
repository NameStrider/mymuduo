#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop) : m_loop(loop) {}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const {
	auto it = m_channelmap.find(channel->fd());
	return it != m_channelmap.end() && it->second == channel;
}