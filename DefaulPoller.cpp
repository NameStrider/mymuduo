/**
 * 本文件就是实现 Poller* newDefaultPoller() 为什么不在 Poller.cpp 文件中实现？因为 Poller 
 * 是抽象基类，pollPoller和epollPoller是其派生。如果在 Poller.cpp 文件中实现就需要包含 pollPoller.h
 * 以及 epollPoller.h ，也就是基类文件需要包含派生类文件，这正好颠倒了，不合理
 */
#include <stdlib.h>
#include "Poller.h"
#include "EpollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop) {
	if (::getenv("MUDOU_USE_POLL")) {
		return NULL; //poll具体实现
	}
	else {
		return new EpollPoller(loop); //返回EpollPoller实例
	}
}