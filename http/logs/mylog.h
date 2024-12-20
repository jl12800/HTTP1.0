#ifndef __M_MYLOG_H__
#define __M_MYLOG_H__

#include "logger.hpp"
namespace mylog
{
    // 1. 提供获取指定日志器的全局接口（避免用户自己操作单例对象）
    Logger::ptr getLogger(const std::string &name)
    {
        return mylog::LoggerManager::getInstance().getLogger(name);
    }
    Logger::ptr rootLogger()
    {
        return mylog::LoggerManager::getInstance().rootLogger();
    }

    // 2. 使用宏函数对日志器的接口进行代理（代理模式）
    #define debug(fmt, ...) debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define info(fmt, ...) info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define warn(fmt, ...) warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define error(fmt, ...) error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define fatal(fmt, ...) fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

    // 3. 提供宏函数，直接通过默认日志器进行日志的标准输出打印（不用获取日志器了）
    #define DEBUG(fmt, ...) mylog::rootLogger()->debug(fmt, ##__VA_ARGS__)
    #define INFO(fmt, ...) mylog::rootLogger()->info(fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...) mylog::rootLogger()->warn(fmt, ##__VA_ARGS__)
    #define ERROR(fmt, ...) mylog::rootLogger()->error(fmt, ##__VA_ARGS__)
    #define FATAL(fmt, ...) mylog::rootLogger()->fatal(fmt, ##__VA_ARGS__)
}

#endif