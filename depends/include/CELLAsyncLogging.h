#ifndef ASYNC_LOGGING_H
#define ASYNC_LOGGING_H

#include "CELLNonCopyable.h"
#include "CELLFixedBuffer.h"
#include "CELLLogStream.h"
#include "CELLLogFile.h"
#include "CELLThread.h"

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

class AsyncLogging
{
public:
    AsyncLogging(const std::string& basename,
        off_t rollSize,
        int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    // 前端调用 append 写入日志
    void append(const char* logling, int len);

    void start()
    {
        running_ = true;
        thread_.Start(nullptr, [this](CELLThread *) -> void {
            threadFunc();
        });
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        //这里？会不会有问题呢？
        thread_.Exit();
    }

private:
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    void threadFunc();

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    CELLThread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

#endif // ASYNC_LOGGING_H