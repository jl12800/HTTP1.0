#pragma once

#include <iostream>
#include <queue>
#include <pthread.h> //同步互斥
#include "logs/mylog.h"
#include "Task.hpp"

#define NUM 6

class ThreadPool
{
private:
    int num; // 线程数目
    bool stop;
    std::queue<Task> task_queue; // 任务队列
    pthread_mutex_t lock;        // 锁
    pthread_cond_t cond;         // 条件变量

    // 构造函数：默认线程数量是5
    ThreadPool(int _num = NUM) : num(_num), stop(false)
    {
        pthread_mutex_init(&lock, nullptr); // 初始化
        pthread_cond_init(&cond, nullptr);
    }

    ThreadPool(const ThreadPool &) {}

    // 线程池单例
    static ThreadPool *single_instance;

public:
    // 获取单例
    static ThreadPool *getinstance()
    {
        static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
        if (single_instance == nullptr)
        {
            pthread_mutex_lock(&_mutex);
            if (single_instance == nullptr)
            {
                single_instance = new ThreadPool();
                single_instance->InitThreadPool();
            }
            pthread_mutex_unlock(&_mutex);
        }
        return single_instance;
    }

    bool IsStop() // 线程是否退出
    {
        return stop;
    }


    bool TaskQueueIsEmpty()
    {
        return task_queue.size() == 0 ? true : false;
    }


    void Lock() // 加锁
    {
        pthread_mutex_lock(&lock);
    }

    void Unlock() // 解锁
    {
        pthread_mutex_unlock(&lock);
    }

    void ThreadWait() // 线程等待
    {
        pthread_cond_wait(&cond, &lock);
    }

    void ThreadWakeup() // 线程唤醒
    {
        pthread_cond_signal(&cond);
    }

    // 线程要执行的方法必须是静态的，因为它默认的参数只有一个
    static void *ThreadRoutine(void *args) /// 线程处理
    {
        ThreadPool *tp = (ThreadPool *)args; // 要读取的线程池的指针

        while (true)
        {
            Task t;
            tp->Lock(); /// 下面的while防止伪唤醒
            while (tp->TaskQueueIsEmpty())
            {
                tp->ThreadWait(); // 当我醒来的时候，一定是占有互斥锁的！
            }
            tp->PopTask(t);
            tp->Unlock();
            t.ProcessOn(); // 处理任务，回调
        }
    }

    bool InitThreadPool() // 1.初始化
    {
        for (int i = 0; i < num; i++)
        {
            pthread_t tid;
            if (pthread_create(&tid, nullptr, ThreadRoutine, this) != 0)
            {
                FATAL("%s", "create thread pool error!");
                return false;
            }
        }
        INFO("%s", "create thread pool success!");
        return true;
    }

    void PushTask(const Task &task) // in2.推送到任务队列
    {
        Lock();
        task_queue.push(task);
        Unlock();
        ThreadWakeup();
    }

    void PopTask(Task &task) // out3.拿出任务从队列
    {
        task = task_queue.front();
        task_queue.pop();
    }

    ~ThreadPool() 
    {
        pthread_mutex_destroy(&lock); // 释放
        pthread_cond_destroy(&cond);
    }
};

ThreadPool *ThreadPool::single_instance = nullptr;