#ifndef __UTIL_LOG_H__
#define __UTIL_LOG_H__

#include <stdio.h>
#include <time.h>

#ifdef __GNUC__
    #define __UNUSED__ __attribute__((__unused__))
#else
    #define __UNUSED__
#endif

#define __GNU_SOURCE
extern char *program_invocation_name;

#define LOG_COLOR_BLUE "\x1b[34m"
#define LOG_COLOR_RED "\x1b[31m"
#define LOG_COLOR_GREEN "\x1b[32m"
#define LOG_COLOR_END "\x1b[0m"

#define LOGFMT(COLOR, fmt, ...) do { fprintf(stderr, "%s:[%s:%d][%s] " fmt "\n", program_invocation_name, __FILE__,  __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define ERR(...) LOGFMT(LOG_COLOR_RED, ##__VA_ARGS__)
#define LOG(...) LOGFMT(LOG_COLOR_GREEN, ##__VA_ARGS__)

#define RET_IF(VAL, ...) do { if (VAL) { ERR(#VAL " is true"); return __VA_ARGS__; } } while (0);

#define DIFF_TIMESPEC(t0, t1, t) do { \
    if (t0.tv_nsec < t1.tv_nsec) {\
        t.tv_sec = (long)t1.tv_sec - t0.tv_sec; \
        t.tv_nsec = t1.tv_nsec - t0.tv_nsec; \
    } else {\
        t.tv_sec = (long)t1.tv_sec - t0.tv_sec - 1, \
        t.tv_nsec = t1.tv_nsec - t0.tv_nsec + 1000000000; \
    } \
} while(0);

#define GETTIME_INIT() \
    struct timespec _timerspec_start; \
    struct timespec _timerspec_end;

#define GETTIME_START() do { \
    clock_gettime(CLOCK_MONOTONIC, &_timerspec_start); \
} while (0);

#define GETTIME_END() do { \
    clock_gettime(CLOCK_MONOTONIC, &_timerspec_end); \
    if (_timerspec_start.tv_nsec < _timerspec_end.tv_nsec) {\
        LOG("Execution time: %ld %lf", \
                (long)_timerspec_end.tv_sec - _timerspec_start.tv_sec, \
                (_timerspec_end.tv_nsec - _timerspec_start.tv_nsec)/(double)1000000000); \
    } else {\
        LOG("Execution time: %ld %lf", \
                (long)_timerspec_end.tv_sec - _timerspec_start.tv_sec - 1, \
                (_timerspec_end.tv_nsec - _timerspec_start.tv_nsec + 1000000000)/(double)1000000000); } \
} while (0);

#endif
