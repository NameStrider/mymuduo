#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include "Buffer.h"

/**
 * 从fd中读取数据，写入缓冲区。但是TCP是基于字节流的，并不知道fd中可读的数据有多少，所以Buffer可能
 * 不够。可以预先开辟一块栈上的内存作为 中间缓存 ，读完后判断是否用到了中间缓存 ，如果用到了就扩容
 * Buffer
 */
ssize_t Buffer::readFd(int fd, int* savedErrno) {
	char extraBuffer[65536] = { 0 }; //栈上的内存
	struct iovec vec[2];
	size_t writeBytes = writableBytes(); //buffer剩余的可写缓冲区的长度

	vec[0].iov_base = begin() + m_writeIndex;
	vec[0].iov_len = writeBytes;
	vec[1].iov_base = extraBuffer;
	vec[1].iov_len = sizeof(extraBuffer);

	int iovcnt = (writeBytes < sizeof(extraBuffer)) ? 2 : 1;
	ssize_t n = ::readv(fd, vec, iovcnt);
	if (n < 0) {
		*savedErrno = errno;
	}
	else if (n <= writeBytes) {
		m_writeIndex += n; //只用了buffer缓存
	}
	else {
		m_writeIndex = m_buffer.size(); //用到了extraBuffer
		append(extraBuffer, n - writeBytes);
	}

	return n;
}

ssize_t Buffer::writefd(int fd, int* savedErrno) {
	ssize_t n = ::write(fd, peek(), readableBytes());
	if (n < 0) {
		*savedErrno = errno;
	}
	return n;
}