#include <time.h>
#include "TimeStamp.h"

TimeStamp::TimeStamp() : m_microSecondsSinceEpoch(0) {}

TimeStamp::TimeStamp(int64_t microSecondsSinceEpoch)
	: m_microSecondsSinceEpoch(microSecondsSinceEpoch) {}

//��ȡ��¼��ǰʱ���TimeStamp����
TimeStamp TimeStamp::now() {
	return TimeStamp(time(NULL)); //�����������ֵ��һ��������������û���ָ�룬��ô�ᷢ�Ϳ�������
}

//��ȡ��¼��ǰʱ���string
//time(NULL)��ȡ��ǰʱ�����localtime(time_t*)��ʱ���תΪ ������ʱ���� ���ݸ�ʽ�Ľṹ��
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