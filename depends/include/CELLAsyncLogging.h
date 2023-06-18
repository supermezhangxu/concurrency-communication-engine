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

    // ǰ�˵��� append д����־
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
        //����᲻���������أ�
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