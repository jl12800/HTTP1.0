#pragma once

#include "logs/mylog.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define BACKLOG 5 // 定义连接队列的最大长度为 5

class TcpServer
{
private:
    int port;              // 保存服务器的端口号
    int listen_sock;       // 服务器监听套接字
    static TcpServer *svr; // 静态指针，指向类的单例实例

private:
    // 私有构造函数，防止外部创建实例，默认端口号为 PORT
    TcpServer(int _port) : port(_port), listen_sock(-1) {}

    // 私有拷贝构造函数，禁止复制对象
    TcpServer(const TcpServer &s) {}

public:
    // 获取 TcpServer 单例实例的静态方法
    static TcpServer *getinstance(int port)
    {
        // 使用静态局部变量的方式实现线程安全的懒汉单例模式:在第一次需要时才创建实例。
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // 初始化互斥锁
        if (nullptr == svr)                                      // 检查实例是否已存在
        {
            pthread_mutex_lock(&lock); // 加锁，确保线程安全
            if (nullptr == svr)
            {
                svr = new TcpServer(port); // 创建新的 TcpServer 实例
                svr->InitServer();         // 初始化服务器
            }
            pthread_mutex_unlock(&lock); // 解锁
        }
        return svr; // 返回单例实例
    }

    // 初始化服务器的方法，包括创建套接字、绑定端口和监听
    void InitServer()
    {
        Socket(); // 创建套接字
        Bind();   // 绑定到指定端口
        Listen(); // 开始监听
    }

    // 创建套接字的方法
    void Socket()
    {
        listen_sock = socket(AF_INET, SOCK_STREAM, 0); // 使用 IPv4, TCP 协议
        if (listen_sock < 0)                           // 判断套接字是否创建成功
        {
            FATAL("%s", "创建套接字失败...");
            exit(1); // 创建失败，退出程序
        }
        int opt = 1;
        // 设置套接字选项，启用端口地址复用
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    // 绑定套接字到指定端口
    void Bind()
    {
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));   // 初始化地址结构体
        local.sin_family = AF_INET;         // 设置为 IPv4 地址
        local.sin_port = htons(port);       // 绑定端口号
        local.sin_addr.s_addr = INADDR_ANY; // 绑定到本机所有可用 IP

        // 绑定套接字到本地地址和端口
        if (bind(listen_sock, (struct sockaddr *)&local, sizeof(local)) < 0)
        {
            FATAL("%s", "绑定套接字失败...");
            exit(2); // 绑定失败，退出程序
        }
    }

    // 开始监听，等待客户端连接
    void Listen()
    {
        if (listen(listen_sock, BACKLOG) < 0) // 开始监听，最大连接队列长度为 BACKLOG
        {
            FATAL("%s", "监听套接字失败...");
            exit(3); // 监听失败，退出程序
        }
    }

    int Sock()
    {
        return listen_sock;
    }

    // 析构函数
    ~TcpServer()
    {
        if (listen_sock >= 0)
            close(listen_sock);
    }
};

// 静态成员初始化为 nullptr
TcpServer *TcpServer::svr = nullptr;