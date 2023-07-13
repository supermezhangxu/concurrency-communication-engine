#include "CELLAsyncLogging.h"
#include "CELLTimestampForLogger.h"

#include <stdio.h>

AsyncLogging::AsyncLogging(const std::string& basename,
    off_t rollSize,
    int flushInterval)
    : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),    
    mutex_(),
    cond_(),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)
{
    // lock在构造函数中自动绑定它的互斥体并加锁，在析构函数中解锁，大大减少了死锁的风险
    std::lock_guard<std::mutex> lock(mutex_);
    // 缓冲区剩余空间足够则直接写入
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        // 当前缓冲区空间不够，将新信息写入备用缓冲区
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            // 备用缓冲区也不够时，重新分配缓冲区，这种情况很少见
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        // 唤醒写入磁盘得后端线程
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    //latch_.countDown();
    LogFile output(basename_, rollSize_, false); // only called by this thread, so no need to use thread safe
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    static const int kBuffersToWriteMaxSize = 25;

    buffersToWrite.reserve(16); // FIXME: why 16?
    while (running_)
    {
        // ensure empty buffer
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        // ensure buffersToWrite is empty
        assert(buffersToWrite.empty());

        { // push buffer to vector buffersToWrite
            std::unique_lock<std::mutex> guard(mutex_);
            if (buffers_.empty())
            { // unusual usage!
                cond_.wait_for(guard, std::chrono::seconds(3)); // wait condition or timeout
            }
            // not empty or timeout

            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        // ensure buffersToWrite is not empty
        assert(!buffersToWrite.empty());

        if (buffersToWrite.size() > kBuffersToWriteMaxSize) // FIXME: why 25? 25x4MB = 100MB, 也就是说, 从上次loop到本次loop已经堆积超过100MB, 就丢弃多余缓冲
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "Dropped log message at %s, %zd larger buffers\n",
                Timestamp::now().toFormattedString().c_str(),
                buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end()); // keep 2 buffer
        }

        // append buffer content to logfile
        for (const auto& buffer : buffersToWrite)
        {
            // FIXME: use unbuffered stdio FILE? or use ::writev ?
            output.append(buffer->data(), buffer->length());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        // move vector buffersToWrite's last buffer to newBuffer1
        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset(); // reset buffer
        }

        // move vector buffersToWrite's last buffer to newBuffer2
        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset(); // reset buffer
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
