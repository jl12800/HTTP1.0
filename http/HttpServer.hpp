#pragma once

#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "logs/mylog.h"
#include "TcpServer.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"

#define PORT 8081

class HttpServer
{
private:
    int port; // 通过TCPserver绑定
    bool stop;

public:
    HttpServer(int _port = PORT) : port(_port), stop(false)
    {
    }

    void InitServer()
    {
        // 信号SIGPIPE需要进行忽略，如果不忽略，在写入时候，可能直接崩溃server
        // 即不忽略 SIGPIPE 信号，服务器在向已关闭的连接写入时会收到这个信号并立即崩溃。
        signal(SIGPIPE, SIG_IGN);
    }

    // 在一个无限循环中，持续监听客户端的连接请求，一旦有客户端连接，就将其交给线程池中的线程来处理。
    void Loop()
    {
        TcpServer *tsvr = TcpServer::getinstance(port);
        INFO("%s", "Loop begin");
        while (!stop)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            int sock = accept(tsvr->Sock(), (struct sockaddr *)&peer, &len); // 获取新连接
            if (sock < 0)
            {
                continue; // 获取套接字失败
            }
            INFO("%s", "Get a new link");

            // 获取一个任务
            Task task(sock);

            // 推送到任务队列
            ThreadPool::getinstance()->PushTask(task);
        }
    }

    ~HttpServer()
    {
    }
};
