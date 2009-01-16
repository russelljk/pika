/*
 *  PProfiler.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PROFILER_HEADER
#define PIKA_PROFILER_HEADER

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class Timer
{
    LARGE_INTEGER start, end;
    LARGE_INTEGER freq;
public:

    INLINE Timer() {}

    INLINE ~Timer() {}

    INLINE void Start()
    {
        static int first = 1;

        if (first)
        {
            QueryPerformanceFrequency(&freq);
            first = 0;
        }
        QueryPerformanceCounter(&start);
    }

    INLINE void End() { QueryPerformanceCounter(&end); }

    INLINE double Val() { return((double)end.QuadPart - (double)start.QuadPart) / ((double)freq.QuadPart); }
};
#elif defined(PIKA_POSIX)

#include <sys/time.h>

class Timer
{
public:
    struct timeval start, end;
    struct timezone tz;


    Timer() {}

    ~Timer() {}

    void Start() 
    { 
        if (gettimeofday(&start, &tz) != 0)
        {
        }
    }

    void End()
    {     
        if (gettimeofday(&end, &tz) != 0)
        {
        }
    }

    double Val()
    {
        double t1 = (double)start.tv_sec + (double)start.tv_usec / (1000000.0);
        double t2 = (double)end.tv_sec   + (double)end.tv_usec  / (1000000.0);
        return t2 - t1;
    }
};
#endif

#endif
