#ifndef __M_LEVEL_H__
#define __M_LEVEL_H__
/* 日志等级模块：
                1. 定义日志系统所包含的所有日志等级
                    UNKNOW = 0,
                    DEBUG,
                    INFO,
                    WARN,
                    ERROR,
                    FATAL,
                    OFF,
                    每个项目中都会设置一个默认的日志输出等级，只有输出的日志等级大于等于默认限制的等级才会进行输出
                2. 提供一个接口，将对应等级的枚举，转换成一个对应的字符串  */
namespace mylog
{
    class LogLevel
    {
    public:

        enum class value
        {
            UNKNOW = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            OFF
        };

        static const char* tostring(LogLevel::value level){
            switch (level)
            {
            case LogLevel::value::DEBUG: return "DEBUG";
            case LogLevel::value::INFO: return "INFO";
            case LogLevel::value::WARN: return "WARN";
            case LogLevel::value::ERROR: return "ERROR";
            case LogLevel::value::FATAL: return "FATAL";
            case LogLevel::value::OFF: return "OFF";
            }
            return "UNKNOW";
        };
    };

}

#endif