#ifndef __NONCOPYABLE_H_
#define __NONCOPYABLE_H_

/**
 * noncopyable被继承后，派生对象可以正常构造和析构，但是无法进行拷贝构造和赋值操作
 */
class noncopyable {
public:
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator= (const noncopyable&) = delete;
protected:
	noncopyable() = default;
	~noncopyable() = default;
};

#endif // !NONCOPYABLE_H
