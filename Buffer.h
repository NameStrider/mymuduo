#ifndef __BUFFER_H_
#define __BUFFER_H_

#include <vector>
#include <string>
#include <algorithm>

//缓冲区类
class Buffer {
public:
	static const size_t kCheapPrepend = 8;
	static const size_t kInitialSize = 1024;

	explicit Buffer(size_t initialSize = kInitialSize) 
		: m_buffer(kCheapPrepend + initialSize),
		  m_readIndex(kCheapPrepend),
		  m_writeIndex(kCheapPrepend) 
	{}

	//可读数据长度
	size_t readableBytes() const {
		return m_writeIndex - m_readIndex;
	}

	//可写数据长度
	size_t writableBytes() const {
		return m_buffer.size() - m_writeIndex;
	}

	void append(const char* data, size_t len) {
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		m_writeIndex += len;
	}

	void retrive(size_t len) {
		//可读数据区中的数据还没读完
		if (len < readableBytes()) {
			m_readIndex += len;
		}
		//全部读完
		else {
			retriveAll();
		}
	}

	//把缓冲区可读数据转成 string
	std::string retriveAllAsString() {
		return retriveAsString(readableBytes());
	}

	//将fd中的数据读到缓冲区
	ssize_t readFd(int fd, int* savedErrno);

	//将缓冲区中的可读数据发送到fd
	ssize_t writefd(int fd, int* savedErrno);
private:
	//对缓冲区的操作是以C风格的数组进行的，但是容器无法直接得到元素地址
	char* begin() {
		//it.operator*(), it.operator&(), 妙哉
		return &*m_buffer.begin(); //获取容器中的数组的首元素的地址
	}

	const char* begin() const {
		return &*m_buffer.begin();
	}

	//返回缓冲区中可读数据区的首地址
	const char* peek() const {
		return begin() + m_readIndex;
	}

	size_t prependableBytes() {
		return m_readIndex;
	}

	//画图理解
	void makeSpace(size_t len) {
		if (writableBytes() + prependableBytes() - kCheapPrepend < len) {
			m_buffer.resize(m_writeIndex + len);
		}
		else {
			size_t readBytes = readableBytes();
			std::copy(begin() + m_readIndex, begin() + m_writeIndex, begin() + kCheapPrepend);
			m_readIndex = kCheapPrepend;
			m_writeIndex = m_readIndex + readBytes;
		}
	}

	void retriveAll() {
		m_writeIndex = m_readIndex = kCheapPrepend;
	}

	//将缓冲区可读数据转成 string
	std::string retriveAsString(size_t len) { //not const std::string&
		std::string result(peek(), len); //以指针+长度的参数类型构造string
		retrive(len); //缓冲区数据已经读取了，接下来需要对其复位(指针更新)
		return result; //返回局部变量，不能返回地址或引用，只能返回值(拷贝)
	}

	//case: buffer.size() - m_writeIndex < len
	void ensureWritableBytes(size_t len) {
		if (writableBytes() < len) {

			//buffer不够，要扩容
			makeSpace(len);
		}
	}

	char* beginWrite() {
		return begin() + m_writeIndex;
	}

	const char* beginWrite() const {
		return begin() + m_writeIndex;
	}

	//用容器管理缓冲区方便扩容
	std::vector<char> m_buffer; 
	size_t m_readIndex;
	size_t m_writeIndex;
};

#endif // !__BUFFER_H_
