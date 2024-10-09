/*实现异步日志缓冲区*/
#ifndef __M_BUFFER_H__
#define __M_BUFFER_H__

#include "utill.hpp"
#include <vector>
#include <cassert>

namespace mylog
{
#define DEFAULT_BUFFER_SIZE (1 * 1024 * 1024)
#define THRESHOLD_BUFFER_SIZE (5 * 1024 * 1024)
#define INCREMENT_BUFFER_SIZE (1 * 1024 * 1024)
    class Buffer
    {
    public:
        Buffer() : _buffer(DEFAULT_BUFFER_SIZE), _writer_idx(0), _reader_idx(0) {}

        // 向缓冲区写入数据
        void push(const char *data, size_t len)
        {
            // // 缓冲区剩余空间不够：1.扩容 2.阻塞/返回false
            // // 1. 固定大小，则直接返回
            // // if(len>writeAbleSize())return;
            // // 2. 动态空间，用于极限性能测试--扩容
            // ensureEnoughSize(len);

            // 0. 考虑空间不够则扩容
            ensureEnoughSize(len);

            // 1. 将数据拷贝到缓冲区
            std::copy(data, data + len, &_buffer[_writer_idx]);
            // 2. 将当前写入位置向后偏移
            moveWriter(len);
        }

        // 返回可读数据的起始地址
        const char *begin()
        {
            return &_buffer[_reader_idx];
        }

        // 返回可读数据的长度
        size_t readAbleSize()
        {
            // 因为当前缓冲区并不是循环缓冲区，不存在空间循环使用情况
            return (_writer_idx - _reader_idx);
        }

        size_t writeAbleSize()
        {
            // 对于扩容思路，不存在可写空间大小，因为总是可写
            // 因此该接口仅仅针对固定大小缓冲区提供
            return (_buffer.size() - _writer_idx);
        }

        // 对读写指针进行向后偏移操作

        void moveReader(size_t len)
        {
            assert(len <= readAbleSize());
            _reader_idx += len;
        }
        // 重置读写位置，初始化缓冲区
        void reset()
        {
            _reader_idx = 0;
            _writer_idx = 0; // 缓冲区所有空间都是空闲
        }
        // 对buffer实现交换操作
        void swap(Buffer &buffer)
        {
            _buffer.swap(buffer._buffer);
            std::swap(_reader_idx, buffer._reader_idx);
            std::swap(_writer_idx, buffer._writer_idx);
        }
        // 判断缓冲区是否为空
        bool empty()
        {
            return (_reader_idx == _writer_idx);
        }

    private:
        // 对空间进行扩容
        void ensureEnoughSize(size_t len)
        {
            if (len <= writeAbleSize())
                return; // 不需要扩容
            size_t new_size = 0;
            if (_buffer.size() < THRESHOLD_BUFFER_SIZE)
            {
                new_size = _buffer.size() * 2 + len; // 小于阈值则翻倍增长
            }
            else
            {
                new_size = _buffer.size() + INCREMENT_BUFFER_SIZE + len; // 线性增长
            }
            _buffer.resize(new_size);
        }

        // 对读写指针进行向后偏移操作
        void moveWriter(size_t len)
        {
            assert((len + _writer_idx) <= _buffer.size());
            _writer_idx += len;
        }

    private:
        std::vector<char> _buffer;
        size_t _reader_idx; // 当前可读数据的指针--本质上是下标
        size_t _writer_idx; // 当前可写数据的指针
    };

}

#endif