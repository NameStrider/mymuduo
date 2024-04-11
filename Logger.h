#ifndef __LOGGER_H_
#define __LOGGER_H_

#include <stdlib.h>
#include <string>
#include "noncopyable.h"

//LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(LogMsgFmt, ...) \
			do{ \
				Logger& logger = Logger::instance(); \
				logger.setLogLevel(INFO); \
				char buffer[1024] = { 0 }; \
				snprintf(buffer, 1024, LogMsgFmt, ##__VA_ARGS__); \
				logger.log(buffer); \
			} while(0)

#define LOG_ERROR(LogMsgFmt, ...) \
			do{ \
				Logger& logger = Logger::instance(); \
				logger.setLogLevel(ERROR); \
				char buffer[1024] = { 0 }; \
				snprintf(buffer, 1024, LogMsgFmt, ##__VA_ARGS__); \
				logger.log(buffer); \
			} while(0)

#define LOG_FATAL(LogMsgFmt, ...) \
			do{ \
				Logger& logger = Logger::instance(); \
				logger.setLogLevel(FATAL); \
				char buffer[1024] = { 0 }; \
				snprintf(buffer, 1024, LogMsgFmt, ##__VA_ARGS__); \
				logger.log(buffer); \
				exit(-1); \
			} while(0)

#ifdef DEBUG_OUT
#define LOG_DEBUG(LogMsgFmt, ...) \
			do{ \
				Logger& logger = Logger::instance(); \
				logger.setLogLevel(DEBUG); \
				char buffer[1024] = { 0 }; \
				snprintf(buffer, 1024, LogMsgFmt, ##__VA_ARGS__); \
				logger.log(buffer); \
			} while(0)
#else 
#define LOG_DEBUG(LogMsgFmt, ...)
#endif
/**
 * 日志组成：
 * 日志级别：INFO ERROR FATAL DEBUG
 * 日志类(单例模式)
 */
enum LogLevel {
	INFO,  //普通运行信息
	ERROR, //错误信息 
	FATAL, //异常信息
	DEBUG, //调试信息
};

class Logger : public noncopyable {
public:
	//单例模式创建Logger对象
	static Logger& instance();

	//设置日志级别
	void setLogLevel(int level);

	//写日志
	void log(const std::string& msg);
private:
	int m_level;
};

#endif // !__LOGGER_H_
