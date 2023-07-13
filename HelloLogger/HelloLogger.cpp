#include "CELLAsyncLogging.h"
#include "CELLLogging.h"
#include "CELLTimestampForLogger.h"

#include <stdio.h>
#include <thread>
#include <chrono>

static const off_t kRollSize = 1 * 1024 * 1024;

AsyncLogging* g_asyncLog = NULL;

inline AsyncLogging* getAsyncLog()
{
    return g_asyncLog;
}

void test_Logging()
{
    LOG_DEBUG << "debug";
    LOG_INFO << "info";
    LOG_WARN << "warn";
    LOG_ERROR << "error";
    // 注意不能轻易使用 LOG_FATAL, LOG_SYSFATAL, 会导致程序abort

    const int n = 10;
    for (int i = 0; i < n; ++i) {
        LOG_INFO << "Hello, " << i << " abc...xyz";
    }
}

void test_AsyncLogging()
{
    const int n = 10;
    //const int n = 10;
    for (int i = 0; i < n; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        LOG_INFO << "Hello, " << i << " abc...xyz";
    }
}

void asyncLog(const char* msg, int len)
{
    AsyncLogging* logging = getAsyncLog();
    if (logging)
    {
        logging->append(msg, len);
    }
}

int main(int argc, char* argv[])
{
    //printf("pid = %d\n", getpid());

    AsyncLogging log("helloAsync", kRollSize);
    //test_Logging();

    //
    std::this_thread::sleep_for(std::chrono::seconds(1));

    g_asyncLog = &log;
    Logger::setOutput(asyncLog); // 为Logger设置输出回调, 重新配接输出位置
    log.start();

    //test_Logging();
    test_AsyncLogging();

    std::this_thread::sleep_for(std::chrono::seconds(10));
    log.stop();
    return 0;
}
