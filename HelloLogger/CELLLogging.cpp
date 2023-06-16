#include "CELLLogging.h"

#include "CELLCurrentThread.h"

#include <Windows.h>

// ���� TLS ����
DWORD tlsIndex;

// ������Ҫ�洢�� TLS �е����ݽṹ
struct ThreadInfo
{
    char t_errnobuf[512];
    char t_time[64] = {0};
    time_t t_lastSecond;
};

// ��ȡ��ǰ������Ϣ
const char* getErrnoMsg(int savedErrno)
{
    // ��ȡ��ǰ�̵߳� TLS ���ݿ飬�������ڣ��򴴽��µ� TLS ���ݿ�
    ThreadInfo* info = (ThreadInfo*)TlsGetValue(tlsIndex);
    if (info == nullptr)
    {
        info = new ThreadInfo();
        TlsSetValue(tlsIndex, info);
    }

    // ��������ת��Ϊ��Ӧ�Ĵ�����Ϣ�����洢���ֲ߳̾��洢������
    strerror_s(info->t_errnobuf, sizeof(info->t_errnobuf), savedErrno);

    return info->t_errnobuf;
}

// ����Level����Level����
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
    // ����� -> time
    formatTime();
    // д����־�ȼ�
    stream_ << GeneralTemplate(getLevelName[level], 6);
    // TODO:error
    if (savedErrno != 0)
    {
        stream_ << getErrnoMsg(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

// Timestamp::toString������˼·��ֻ����������Ҫ�������
void Logger::Impl::formatTime()
{
    Timestamp now = Timestamp::now();
    time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);

    //struct tm* tm_time = nullptr;
    //tm_time = localtime(&seconds);
    struct tm tm_time;
    gmtime_s(&tm_time, &seconds);

    // ��ȡ��ǰ�̵߳� TLS ���ݿ飬�������ڣ��򴴽��µ� TLS ���ݿ�
    ThreadInfo* info = (ThreadInfo*)TlsGetValue(tlsIndex);
    if (info == nullptr)
    {
        info = new ThreadInfo();
        TlsSetValue(tlsIndex, info);
    }

    // д����̴߳洢��ʱ��buf��
    snprintf(info->t_time , sizeof(ThreadInfo::t_time), "%4d/%02d/%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec);
    // �������һ��ʱ�����
    info->t_lastSecond = seconds;

    // muduoʹ��Fmt��ʽ����������������ֱ��д��buf
    char buf[32] = { 0 };
    snprintf(buf, sizeof(buf), "%06d ", microseconds);

    // ���ʱ�䣬����΢��(֮ǰ��(buf, 6),����һ���ո�)
    stream_ << GeneralTemplate(info->t_time, 17) << GeneralTemplate(buf, 7);
}

void Logger::Impl::finish()
{
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_)
        << ':' << line_ << '\n';
}

// levelĬ��ΪINFO�ȼ�
Logger::Logger(const char* file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(const char* file, int line, Logger::LogLevel level)
    : impl_(level, 0, file, line)
{
}

// ���Դ�ӡ���ú���
Logger::Logger(const char* file, int line, Logger::LogLevel level, const char* func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}


Logger::~Logger()
{
    impl_.finish();
    // ��ȡbuffer(stream_.buffer_)
    const LogStream::Buffer& buf(stream().buffer());
    // ���(Ĭ�����ն����)
    g_output(buf.data(), buf.length());
    // FATAL�����ֹ����
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
