#ifndef TIME_STAMP_H
#define TIME_STAMP_H

#include <iostream>
#include <string>
#include <chrono>
#include <time.h>

class Timestamp
{
public:
    Timestamp()
        : microSecondsSinceEpoch_(0)
    {
    }

    explicit Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {
    }

    // ��ȡ��ǰʱ���
    static Timestamp now();

    //��std::string��ʽ����,��ʽ[millisec].[microsec]
    std::string toString() const;
    //��ʽ, "%4d��%02d��%02d�� ����%d %02d:%02d:%02d.%06d",ʱ����.΢��
    std::string toFormattedString(bool showMicroseconds = false) const;

    //���ص�ǰʱ�����΢��
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    //���ص�ǰʱ���������
    time_t secondsSinceEpoch() const
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    // ʧЧ��ʱ���������һ��ֵΪ0��Timestamp
    static Timestamp invalid()
    {
        return Timestamp();
    }

    // 1��=1000*1000΢��
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    // ��ʾʱ�����΢����(��epoch��ʼ������΢����)
    int64_t microSecondsSinceEpoch_;
};

/**
 * ��ʱ����Ҫ�Ƚ�ʱ����������Ҫ���������
 */
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

// ������ظ���ʱ����ͻ�Դ�ʱ����������ӡ�
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    // ����ʱ������ת��Ϊ΢��
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    // ��������ʱ���ʱ���
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

#endif // TIME_STAMP_H