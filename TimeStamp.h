#ifndef __TIMESTAMP_H_
#define __TIMESTAMP_H_

#include <string>
#include <iostream>
#include "noncopyable.h"

//时间戳管理类，读写时间(单例模式)
class TimeStamp {
public:
	//默认构造函数
	TimeStamp();

	//重载，参数构造函数，explicit防止隐式转换
	explicit TimeStamp(int64_t microSecondsSinceEpoch);

	//当前TimeStamp
	static TimeStamp now();

	//将时间戳转为string
	std::string tostring() const;
private:
	//时间戳
	int64_t m_microSecondsSinceEpoch;
};

#endif // !__TIMESTAMP_H_
