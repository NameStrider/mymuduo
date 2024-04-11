#include "CurrentThread.h"

namespace CurrentThread 
{
	thread_local int t_cachedTid = 0;

	void cacheTid() {
		if (t_cachedTid == 0) {

			//通过Linux系统调用获取当前线程的tid，不是pthread_self()?
			t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
		}
	}
}