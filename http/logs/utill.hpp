#ifndef __M_UTILL_H__
#define __M_UTILL_H__

/* 实用类接口：
   - 获取系统时间
   - 判断文件是否存在
   - 获取文件的所在目录
   - 创建目录
*/

#include <iostream>
#include <sys/stat.h>  // 用于文件状态信息
#include <ctime>       // 用于获取系统时间
#include <string>      // 用于字符串操作
#include <cstddef>     // 用于定义标准数据类型
#include <cstdio>      // 用于文件操作

using std::size_t;

namespace mylog {
    namespace utill {
        // date 类实现获取当前时间的功能
        class date {
        public:
            // 获取当前时间的静态成员函数
            // 无需创建对象即可调用，如：size_t currentTime = date::now();
            static size_t now() {
                return (size_t)time(nullptr);  // 返回当前时间的 UNIX 时间戳（秒数）
            }
        };

        // file 类实现了一些与文件操作相关的静态成员函数
        class file {
        public:
            // 判断文件是否存在的静态函数
            // 传入文件名称，返回文件是否存在
            static bool exits(const std::string &name) {
                struct stat st;  // 定义一个stat结构体用于存储文件状态信息
                return stat(name.c_str(), &st) == 0;  // 使用stat函数检查文件状态，存在则返回true
            }

            // 获取文件路径的静态函数
            // 传入文件的名称，返回文件所在的目录
            static std::string path(const std::string &name) {
                if (name.empty())
                    return ".";  // 如果传入的是空字符串，则认为文件在当前目录

                // 查找文件路径中的最后一个斜杠（Linux和Windows兼容）
                size_t pos = name.find_last_of("/\\");
                if (pos == std::string::npos)
                    return ".";  // 如果没有找到斜杠，则文件在当前目录

                return name.substr(0, pos + 1);  // 返回文件所在的目录（含斜杠）
            }

            // 创建文件目录的静态函数
            // 传入文件的路径，递归创建目录
            static void creat_dircetory(const std::string &path) {
                size_t pos, idx = 0;

                // 循环处理路径中的每个部分，逐级创建目录
                while (idx < path.size()) {
                    // 查找下一个斜杠的位置，斜杠分隔不同级别的目录
                    pos = path.find_first_of("/\\", idx);
                    if (pos == std::string::npos) {
                        // 如果没有找到下一个斜杠，说明是最后一级目录
                        mkdir(path.c_str(), 0777);  // 直接创建整个路径对应的目录
                        return;
                    }

                    // 获取当前级别的父目录
                    std::string parent_dir = path.substr(0, pos + 1);  // 包括斜杠
                    idx = pos + 1;

                    // 如果父目录已存在，则跳过
                    if (exits(parent_dir) == true) {
                        continue;
                    }

                    // 如果父目录不存在，则创建
                    mkdir(parent_dir.c_str(), 0777);
                }
            }
        };

    }
}

#endif // __M_UTILL_H__
