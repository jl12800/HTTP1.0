#ifndef _M_FORMAT_H__
#define _M_FORMAT_H__

#include "level.hpp"
#include "message.hpp"
#include <memory>
#include <ctime>
#include <vector>
#include <cassert>
#include <sstream>
#include <stdlib.h>

namespace mylog
{
    // 抽象格式化子项基类
    class FormatItem
    {
    public:
        using ptr = std::shared_ptr<FormatItem>;
        virtual void format(std::ostream &out, const LogMsg &msg) = 0;
    };

    // 派生类格式化子项--消息、等级、时间、文件名、行号、线程ID、日志器名、制表符、换行、其他
    class MsgFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg._payload;
        }
    };

    class LevelFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << LogLevel::tostring(msg._level);
        }
    };

    class TimeFormatItem : public FormatItem
    {
    public:
        TimeFormatItem(const std::string &fmt = "%H:%M:%S") : _time_fmt(fmt) {}
        void format(std::ostream &out, const LogMsg &msg) override
        {
            struct tm t;
            localtime_r(&msg._ctime, &t);
            char tmp[32] = {0};
            strftime(tmp, 31, _time_fmt.c_str(), &t);

            out << tmp;
        }

    private:
        std::string _time_fmt; //%H:%M:%S
    };

    class FileFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg._file;
        }
    };

    class LineFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg._line;
        }
    };

    class ThreadFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg._tid;
        }
    };

    class LoggerFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << msg._logger;
        }
    };

    class TabFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << "\t";
        }
    };

    class NlineFormatItem : public FormatItem
    {
    public:
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << "\n";
        }
    };

    class OtherFormatItem : public FormatItem
    {
    public:
        OtherFormatItem(const std::string &str) : _str(str) {}
        void format(std::ostream &out, const LogMsg &msg) override
        {
            out << _str;
        }

    private:
        std::string _str;
    };

    /*  %d 日期        %T  缩进     %t 线程ID   %p 日志级别
        %c 日志器名称   %f 文件名    %l 行号     %m 日志消息    %n 换行
    */
    class Formatter
    {
    public:
        using ptr = std::shared_ptr<Formatter>;
        Formatter(const std::string &pattern = "[%d{%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n")
            : _pattern(pattern)
        {
            assert(parsePattern());
        }

        // 对msg进行格式化
        /*这个内部 format 函数开始执行，遍历 _items 中的每个格式化子项，并依次将格式化后的日志信息写入 ss 中*/
        void format(std::ostream &out, const LogMsg &msg)
        {
            // _items 是一个包含所有格式化子项的向量，代表了用户定义的日志格式化规则的具体实现
            for (auto &item : _items)
            {
                item->format(out, msg); // 对每一个格式化子项调用其 format 函数，并输出到 out
            }
        }

        std::string format(const LogMsg &msg)
        {
            std::stringstream ss; // 创建一个字符串流对象 ss
            format(ss, msg);      // 内部调用 format(std::ostream &out, const LogMsg &msg)，并将 ss 作为输出流传递给它
            // 当内部的 format(std::ostream &out, const LogMsg &msg) 函数执行完毕后，ss 中已经包含了完整的格式化日志内容
            return ss.str(); // 最终通过 ss.str() 将字符串流中的内容转换为 std::string 并返回。
        } /*它创建了一个 std::stringstream 对象 ss，用于临时存储格式化后的日志内容。*/

        /* 关键点总结
            执行顺序：
                先调用外部的 std::string format(const LogMsg &msg)。
                外部函数内部调用 void format(std::ostream &out, const LogMsg &msg)。
                内部函数执行完毕后，外部函数返回最终格式化的字符串。
            作用：

                外部函数提供了一个简便的接口来获取格式化后的日志字符串。
                内部函数负责具体的格式化逻辑，将日志信息按照定义好的规则输出到指定的流（在这个例子中是 std::stringstream）。
            灵活性：
                内部函数使用 std::ostream 作为参数，这使得它可以被用于输出到不同的目标，比如文件流、网络流等，不仅限于字符串流。  */
    private:
        // 对格式化规则字符串进行解析
        bool parsePattern()
        {
            // 1. 对格式化规则字符串解析
            std::vector<std::pair<std::string, std::string>> fmt_order;
            size_t pos = 0;
            std::string key, val;
            while (pos < _pattern.size())
            {
                // 处理原始字符串--判断是否是%，若不是就是原始字符

                if (_pattern[pos] != '%')
                {
                    val.push_back(_pattern[pos++]);
                    continue;
                }

                // 走到这里说明pos位置就是%字符。注意会不会是%%
                if (pos + 1 < _pattern.size() && _pattern[pos + 1] == '%')
                {
                    val.push_back('%');
                    pos += 2;
                    continue;
                }

                // 走到这里，代表%后面就是一个格式化字符，代表原始字符串处理完毕  %c
                if (val.empty() == false)
                {
                    fmt_order.push_back(std::make_pair("", val));
                    val.clear();
                }

                // 接下来对格式化字符进行处理 pos->%
                pos += 1; // pos->格式化字符
                if (pos == _pattern.size())
                {
                    std::cout << "%之后，没有对应的格式化字符串！\n";
                    return false;
                }

                key = _pattern[pos]; // %d{%H%M%S},此时pos=d
                pos += 1;            // 这时pos指向格式化字符串后的位置,此时pos={
                // 处理可能含有的字串
                if (pos < _pattern.size() && _pattern[pos] == '{')
                {
                    pos += 1; // 这时候pos->'{'之后的位置
                    while (pos < _pattern.size() && _pattern[pos] != '}')
                        val.push_back(_pattern[pos++]);

                    // 走到了末尾跳出循环，代表没有遇到'}'即格式是错误的
                    if (pos == _pattern.size())
                    {
                        std::cout << "子规则匹配出错！\n";
                        return false;
                    }
                    // 没有走到末尾跳出循环，说明下一个字符是'}'
                    pos += 1;
                }
                fmt_order.push_back(std::make_pair(key, val));
                key.clear();
                val.clear();
            }

            // 2. 根据解析得到的数据初始化格式化子项数组成员
            for (auto &it : fmt_order)
            {
                _items.push_back(creatItem(it.first, it.second));
            }
            return true;
        }

        // 根据不同的格式化字符创建不同的格式化子项
        FormatItem::ptr creatItem(const std::string &key, const std::string &val)
        {
            if (key == "d")
                return std::make_shared<TimeFormatItem>(val);
            if (key == "t")
                return std::make_shared<ThreadFormatItem>();
            if (key == "c")
                return std::make_shared<LoggerFormatItem>();
            if (key == "f")
                return std::make_shared<FileFormatItem>();
            if (key == "l")
                return std::make_shared<LineFormatItem>();
            if (key == "p")
                return std::make_shared<LevelFormatItem>();
            if (key == "T")
                return std::make_shared<TabFormatItem>();
            if (key == "m")
                return std::make_shared<MsgFormatItem>();
            if (key == "n")
                return std::make_shared<NlineFormatItem>();
            if (key == "")
                return std::make_shared<OtherFormatItem>(val);
            std::cout << "没有对应的格式化字符：%" << key << std::endl;
            abort();
            return FormatItem::ptr();
        }

    private:
        std::string _pattern;                // 格式化规则子项
        std::vector<FormatItem::ptr> _items; // 存储解析后的格式化子项
    };
}

#endif