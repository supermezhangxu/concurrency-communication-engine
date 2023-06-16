#include "CELLLogging.h"

#include "CELLCurrentThread.h"

#include <Windows.h>

// 定义 TLS 索引
DWORD tlsIndex;

// 定义需要存储在 TLS 中的数据结构
struct ThreadInfo
{
    char t_errnobuf[512];
    char t_time[64] = {0};
    time_t t_lastSecond;
};

// 获取当前错误信息
const char* getErrnoMsg(int savedErrno)
{
    // 获取当前线程的 TLS 数据块，若不存在，则创建新的 TLS 数据块
    ThreadInfo* info = (ThreadInfo*)TlsGetValue(tlsIndex);
    if (info == nullptr)
    {
        info = new ThreadInfo();
        TlsSetValue(tlsIndex, info);
    }

    // 将错误码转换为对应的错误信息，并存储到线程局部存储变量中
    strerror_s(info->t_errnobuf, sizeof(info->t_errnobuf), savedErrno);

    return info->t_errnobuf;
}

// 根据Level返回Level名字
const char* getLevelName[Logger::LogLevel::LEVEL_COUNT]
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

Logger::LogLevel initLogLevel()
{
    return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

static void defaultOutput(const char* data, int len)
{
    fwrite(data, len, sizeof(char), stdout);
}

static void defaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(Logger::LogLevel level, int savedErrno, const char* file, int line)
    : time_(Timestamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
    // 输出流 -> time
    formatTime();
    // 写入日志等级
    stream_ << GeneralTemplate(getLevelName[level], 6);
    // TODO:error
    if (savedErrno != 0)
    {
        stream_ << getErrnoMsg(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

// Timestamp::toString方法的思路，只不过这里需要输出到流
void Logger::Impl::formatTime()
{
    Timestamp now = Timestamp::now();
    time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);

    //struct tm* tm_time = nullptr;
    //tm_time = localtime(&seconds);
    struct tm tm_time;
    gmtime_s(&tm_time, &seconds);

    // 获取当前线程的 TLS 数据块，若不存在，则创建新的 TLS 数据块
    ThreadInfo* info = (ThreadInfo*)TlsGetValue(tlsIndex);
    if (info == nullptr)
    {
        info = new ThreadInfo();
        TlsSetValue(tlsIndex, info);
    }

    // 写入此线程存储的时间buf中
    snprintf(info->t_time , sizeof(ThreadInfo::t_time), "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec);
    // 更新最后一次时间调用
    info->t_lastSecond = seconds;

    // muduo使用Fmt格式化整数，这里我们直接写入buf
    char buf[32] = { 0 };
    snprintf(buf, sizeof(buf), "%06d ", microseconds);

    // 输出时间，附有微妙(之前是(buf, 6),少了一个空格)
    stream_ << GeneralTemplate(info->t_time, 17) << GeneralTemplate(buf, 7);
}

void Logger::Impl::finish()
{
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_)
        << ':' << line_ << '\n';
}

// level默认为INFO等级
Logger::Logger(const char* file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(const char* file, int line, Logger::LogLevel level)
    : impl_(level, 0, file, line)
{
}

// 可以打印调用函数
Logger::Logger(const char* file, int line, Logger::LogLevel level, const char* func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}


Logger::~Logger()
{
    impl_.finish();
    // 获取buffer(stream_.buffer_)
    const LogStream::Buffer& buf(stream().buffer());
    // 输出(默认向终端输出)
    g_output(buf.data(), buf.length());
    // FATAL情况终止程序
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}
