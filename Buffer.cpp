#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include "Buffer.h"

/**
 * ��fd�ж�ȡ���ݣ�д�뻺����������TCP�ǻ����ֽ����ģ�����֪��fd�пɶ��������ж��٣�����Buffer����
 * ����������Ԥ�ȿ���һ��ջ�ϵ��ڴ���Ϊ �м仺�� ��������ж��Ƿ��õ����м仺�� ������õ��˾�����
 * Buffer
 */
ssize_t Buffer::readFd(int fd, int* savedErrno) {
	char extraBuffer[65536] = { 0 }; //ջ�ϵ��ڴ�
	struct iovec vec[2];
	size_t writeBytes = writableBytes(); //bufferʣ��Ŀ�д�������ĳ���

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
		m_writeIndex += n; //ֻ����buffer����
	}
	else {
		m_writeIndex = m_buffer.size(); //�õ���extraBuffer
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