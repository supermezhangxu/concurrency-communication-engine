#include "CELLTimestamp.h"
#include <Windows.h>

static const int64_t kWinEpochOffset = 116444736000000000;

// ��ȡ��ǰʱ���
Timestamp Timestamp::now()
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    // �� FILETIME ת��Ϊ 64 λ��������λΪ 100ns���� 1e-7 ��
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;

    // ת��Ϊ΢��
    return Timestamp(u.QuadPart / 10 - kWinEpochOffset);
#else
    struct timeval tv;
    // ��ȡ΢�����
    // ��x86-64ƽ̨gettimeofday()�Ѳ���ϵͳ����,���������ں�, ��ε��ò�����������ʧ.
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    // ת��Ϊ΢��
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);

#endif // _WIN32

}

// 2022/08/26 16:29:10
// 20220826 16:29:10.773804
//std::string Timestamp::toFormattedString(bool showMicroseconds) const
//{
//    char buf[64] = { 0 };
//    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
//    // ʹ��localtime������������ʽ��������ʱ��
//    tm* tm_time = localtime(&seconds);
//    if (showMicroseconds)
//    {
//        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
//        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d.%06d",
//            tm_time->tm_year + 1900,
//            tm_time->tm_mon + 1,
//            tm_time->tm_mday,
//            tm_time->tm_hour,
//            tm_time->tm_min,
//            tm_time->tm_sec,
//            microseconds);
//    }
//    else
//    {
//        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d",
//            tm_time->tm_year + 1900,
//            tm_time->tm_mon + 1,
//            tm_time->tm_mday,
//            tm_time->tm_hour,
//            tm_time->tm_min,
//            tm_time->tm_sec);
//    }
//    return buf;
//}

std::string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[64] = { 0 };
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    // ʹ�� _localtime64_s() ������������ʽ��������ʱ��
    struct tm tm_time;
    _localtime64_s(&tm_time, &seconds);
    
    if (showMicroseconds)
    {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d.%06d",
            tm_time.tm_year + 1900,
            tm_time.tm_mon + 1,
            tm_time.tm_mday,
            tm_time.tm_hour,
            tm_time.tm_min,
            tm_time.tm_sec,
            microseconds);        
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900,
            tm_time.tm_mon + 1,
            tm_time.tm_mday,
            tm_time.tm_hour,
            tm_time.tm_min,
            tm_time.tm_sec);
    }
    std::cout << buf << std::endl;
    return buf;
}

// int main()
// {
//     Timestamp time;
//     std::cout << time.now().toFormattedString() << std::endl;
//     std::cout << time.now().toFormattedString(true) << std::endl;

//     return 0;
// }