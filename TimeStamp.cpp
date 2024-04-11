#include <time.h>
#include "TimeStamp.h"

TimeStamp::TimeStamp() : m_microSecondsSinceEpoch(0) {}

TimeStamp::TimeStamp(int64_t microSecondsSinceEpoch)
	: m_microSecondsSinceEpoch(microSecondsSinceEpoch) {}

//获取记录当前时间的TimeStamp对象
TimeStamp TimeStamp::now() {
	return TimeStamp(time(NULL)); //如果函数返回值是一个对象而不是引用或者指针，那么会发送拷贝构造
}

//获取记录当前时间的string
//time(NULL)获取当前时间戳，localtime(time_t*)将时间戳转为 年月日时分秒 数据格式的结构体
std::string TimeStamp::tostring() const {
	char buffer[128] = { 0 };
	tm* tm_time = localtime(&m_microSecondsSinceEpoch);
	snprintf(buffer, 128, "%4d/%02d/%02d %02d:%02d:%02d",
				tm_time->tm_year + 1900,
				tm_time->tm_mon + 1,
				tm_time->tm_mday,
				tm_time->tm_hour,
				tm_time->tm_min,
				tm_time->tm_sec);
	return buffer;
}

//int main() {
//	std::cout << TimeStamp::now().tostring() << std::endl;
//	return 0;
//}