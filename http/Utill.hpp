#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

// 工具类 Util，用于提供常用的网络操作函数
class Utill
{
public:
    // 从套接字中读取一行数据，并存入字符串 out 中
    // sock: 客户端套接字
    // out: 存放读取到的行数据
    // 返回值：成功时返回读取的字符数，连接关闭时返回 0，出错时返回 -1
    static int ReadLine(int sock, std::string &out)
    {
        char ch = 'X'; // 临时字符，用于接收数据
        while (ch != '\n')
        {                                      // 循环读取直到遇到换行符 '\n'
            ssize_t s = recv(sock, &ch, 1, 0); // 从套接字读取 1 字节数据
            if (s > 0)
            { // 如果读取成功
                if (ch == '\r')
                { // 如果遇到回车符 '\r'
                    // MSG_PEEK 选项：窥探数据，不移除缓冲区内容
                    recv(sock, &ch, 1, MSG_PEEK); // 窥探下一个字符
                    if (ch == '\n')
                    {                          // 如果下一个字符是换行符 '\n'
                        recv(sock, &ch, 1, 0); // 消费掉 '\n'，将 '\r\n' 转换为 '\n'
                    }
                    else
                    {
                        ch = '\n'; // 否则，将 '\r' 转换为 '\n'
                    }
                }
                // 将读取到的字符（普通字符或换行符）加入输出字符串
                out.push_back(ch);
            }
            else if (s == 0)
            { // 如果读取结果为 0，表示连接已关闭
                return 0;
            }
            else
            { // 如果读取结果为负，表示读取过程中出现了错误
                return -1;
            }
        }
        // 返回读取的字符数
        return out.size();
    }

    static bool CutString(const std::string &target, std::string &sub1_out, std::string &sub2_out, std::string sep)
    {
        size_t pos = target.find(sep); // 在目标字符串中查找分隔符的位置
        if (pos != std::string::npos)
        {                                               // 如果找到分隔符
            sub1_out = target.substr(0, pos);           // 提取分隔符前的子字符串
            sub2_out = target.substr(pos + sep.size()); // 提取分隔符后的子字符串
            return true;                                // 分割成功
        }
        return false; // 分割失败
    }
};