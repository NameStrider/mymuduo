#include <iostream>
#include "Logger.h"
#include "TimeStamp.h"

Logger& Logger::instance() {
	static Logger logger;
	return logger;
}

//设置日志级别
void Logger::setLogLevel(int level) {
	m_level = level;
}

//写日志，输出到stdout，格式：[level], [time] : [msg]
void Logger::log(const std::string& msg) {
	//输出日志级别
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

	//输出时间和信息
	std::cout << TimeStamp::now().tostring() << ":" << msg << std::endl;
}