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
    // lock�ڹ��캯�����Զ������Ļ����岢�����������������н������������������ķ���
    std::lock_guard<std::mutex> lock(mutex_);
    // ������ʣ��ռ��㹻��ֱ��д��
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        // ��ǰ�������ռ䲻����������Ϣд�뱸�û�����
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            // ���û�����Ҳ����ʱ�����·��仺����������������ټ�
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        // ����д����̵ú���߳�
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc()
{
    // output��д����̵Ľӿ�
    LogFile output(basename_, rollSize_, false);
    // ��˻����������ڹ黹ǰ�˵û�������currentBuffer nextBuffer
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    // ������������Ϊ16�������ں�ǰ�˻�����������н���
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_)
    {
        {
            // ��������������������߳������ʱ����޷���ǰ��Buffer����д������
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty())
            {
                // �ȴ�����Ҳ��Ӵ�����
                cond_.wait_for(lock, std::chrono::seconds(3));
            }

            // ��ʱ��ʹ�õ�bufferҲ����buffer�����У�ûд��Ҳ�Ž�ȥ������ȴ�̫�ò�ˢ��һ�Σ�
            buffers_.push_back(std::move(currentBuffer_));
            // �黹��ʹ�û�����
            currentBuffer_ = std::move(newBuffer1);
            // ��˻�������ǰ�˻���������
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        // �������� buffer������д���ļ�
        for (const auto& buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }

        // ֻ��������������
        if (buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }

        // �黹newBuffer1������
        if (!newBuffer1)
        {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        // �黹newBuffer2������
        if (!newBuffer2)
        {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear(); // ��պ�˻���������
        output.flush(); //����ļ�������
    }
    output.flush();
}