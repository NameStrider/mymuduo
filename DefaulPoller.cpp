/**
 * ���ļ�����ʵ�� Poller* newDefaultPoller() Ϊʲô���� Poller.cpp �ļ���ʵ�֣���Ϊ Poller 
 * �ǳ�����࣬pollPoller��epollPoller��������������� Poller.cpp �ļ���ʵ�־���Ҫ���� pollPoller.h
 * �Լ� epollPoller.h ��Ҳ���ǻ����ļ���Ҫ�����������ļ��������õߵ��ˣ�������
 */
#include <stdlib.h>
#include "Poller.h"
#include "EpollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop) {
	if (::getenv("MUDOU_USE_POLL")) {
		return NULL; //poll����ʵ��
	}
	else {
		return new EpollPoller(loop); //����EpollPollerʵ��
	}
}