#include "CELLCurrentThread.h"

namespace CurrentThread
{
    __declspec(thread) int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = GetCurrentThreadId();
        }
    }
}