#include <iostream>
#include "Logger.h"
#include "TimeStamp.h"

Logger& Logger::instance() {
	static Logger logger;
	return logger;
}

//������־����
void Logger::setLogLevel(int level) {
	m_level = level;
}

//д��־�������stdout����ʽ��[level], [time] : [msg]
void Logger::log(const std::string& msg) {
	//�����־����
	switch (m_level) {
	case INFO: {
		std::cout << "[INFO]";
		break;
	}

	case ERROR: {
		std::cout << "[ERROR]";
		break;
	}

	case FATAL: {
		std::cout << "[FATAL]";
		break;
	}

	case DEBUG: {
		std::cout << "[DEBUG]";
		break;
	}

	default:
		break;
	}

	//���ʱ�����Ϣ
	std::cout << TimeStamp::now().tostring() << ":" << msg << std::endl;
}