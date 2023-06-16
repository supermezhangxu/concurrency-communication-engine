#ifndef CURRENT_THREAD_H
#define CURRENT_THREAD_H

#ifdef _WIN32
#include <windows.h> // 按需包含头文件

namespace CurrentThread
{
    extern __declspec(thread) int t_cachedTid; // 在Windows平台下使用__declspec(thread)来声明线程本地存储变量

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
    extern __thread int t_cachedTid; // 保存tid缓冲，避免多次系统调用

    void cacheTid();

    // 内联函数
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