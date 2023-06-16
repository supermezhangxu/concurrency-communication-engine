#ifndef CURRENT_THREAD_H
#define CURRENT_THREAD_H

#ifdef _WIN32
#include <windows.h> // �������ͷ�ļ�

namespace CurrentThread
{
    extern __declspec(thread) int t_cachedTid; // ��Windowsƽ̨��ʹ��__declspec(thread)�������̱߳��ش洢����

    void cacheTid();

    inline int tid() {
        if (t_cachedTid == 0) {
            cacheTid();
        }
        return t_cachedTid;
    }
}
#else
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid; // ����tid���壬������ϵͳ����

    void cacheTid();

    // ��������
    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}
#endif // _WIN32_


#endif // CURRENT_THREAD_H