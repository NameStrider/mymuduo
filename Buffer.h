#ifndef __BUFFER_H_
#define __BUFFER_H_

#include <vector>
#include <string>
#include <algorithm>

//��������
class Buffer {
public:
	static const size_t kCheapPrepend = 8;
	static const size_t kInitialSize = 1024;

	explicit Buffer(size_t initialSize = kInitialSize) 
		: m_buffer(kCheapPrepend + initialSize),
		  m_readIndex(kCheapPrepend),
		  m_writeIndex(kCheapPrepend) 
	{}

	//�ɶ����ݳ���
	size_t readableBytes() const {
		return m_writeIndex - m_readIndex;
	}

	//��д���ݳ���
	size_t writableBytes() const {
		return m_buffer.size() - m_writeIndex;
	}

	void append(const char* data, size_t len) {
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		m_writeIndex += len;
	}

	void retrive(size_t len) {
		//�ɶ��������е����ݻ�û����
		if (len < readableBytes()) {
			m_readIndex += len;
		}
		//ȫ������
		else {
			retriveAll();
		}
	}

	//�ѻ������ɶ�����ת�� string
	std::string retriveAllAsString() {
		return retriveAsString(readableBytes());
	}

	//��fd�е����ݶ���������
	ssize_t readFd(int fd, int* savedErrno);

	//���������еĿɶ����ݷ��͵�fd
	ssize_t writefd(int fd, int* savedErrno);
private:
	//�Ի������Ĳ�������C����������еģ����������޷�ֱ�ӵõ�Ԫ�ص�ַ
	char* begin() {
		//it.operator*(), it.operator&(), ����
		return &*m_buffer.begin(); //��ȡ�����е��������Ԫ�صĵ�ַ
	}

	const char* begin() const {
		return &*m_buffer.begin();
	}

	//���ػ������пɶ����������׵�ַ
	const char* peek() const {
		return begin() + m_readIndex;
	}

	size_t prependableBytes() {
		return m_readIndex;
	}

	//��ͼ���
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

	//���������ɶ�����ת�� string
	std::string retriveAsString(size_t len) { //not const std::string&
		std::string result(peek(), len); //��ָ��+���ȵĲ������͹���string
		retrive(len); //�����������Ѿ���ȡ�ˣ���������Ҫ���临λ(ָ�����)
		return result; //���ؾֲ����������ܷ��ص�ַ�����ã�ֻ�ܷ���ֵ(����)
	}

	//case: buffer.size() - m_writeIndex < len
	void ensureWritableBytes(size_t len) {
		if (writableBytes() < len) {

			//buffer������Ҫ����
			makeSpace(len);
		}
	}

	char* beginWrite() {
		return begin() + m_writeIndex;
	}

	const char* beginWrite() const {
		return begin() + m_writeIndex;
	}

	//����������������������
	std::vector<char> m_buffer; 
	size_t m_readIndex;
	size_t m_writeIndex;
};

#endif // !__BUFFER_H_
