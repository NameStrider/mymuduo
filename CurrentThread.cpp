#include "CurrentThread.h"

namespace CurrentThread 
{
	thread_local int t_cachedTid = 0;

	void cacheTid() {
		if (t_cachedTid == 0) {

			//ͨ��Linuxϵͳ���û�ȡ��ǰ�̵߳�tid������pthread_self()?
			t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
		}
	}
}