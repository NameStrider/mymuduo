#ifndef __TIMESTAMP_H_
#define __TIMESTAMP_H_

#include <string>
#include <iostream>
#include "noncopyable.h"

//ʱ��������࣬��дʱ��(����ģʽ)
class TimeStamp {
public:
	//Ĭ�Ϲ��캯��
	TimeStamp();

	//���أ��������캯����explicit��ֹ��ʽת��
	explicit TimeStamp(int64_t microSecondsSinceEpoch);

	//��ǰTimeStamp
	static TimeStamp now();

	//��ʱ���תΪstring
	std::string tostring() const;
private:
	//ʱ���
	int64_t m_microSecondsSinceEpoch;
};

#endif // !__TIMESTAMP_H_
