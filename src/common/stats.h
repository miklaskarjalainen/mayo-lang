#ifndef MAYO_COMMON_STATS_H
#define MAYO_COMMON_STATS_H

#include <time.h>

#define PERF_BEGIN(variable) const clock_t variable = clock()
#define PERF_END(begin_var) (clock() - begin_var)

#define PRINT_DURATION(duration)                                                    \
    do {                                                                            \
        const long int MicroSeconds = (long int)(duration);                         \
        if (MicroSeconds < 1000) {                                                  \
            printf("%lius", MicroSeconds);                                          \
        }                                                                           \
        else if (MicroSeconds < 1000000) {                                          \
            double MilliSeconds = (double)duration / (CLOCKS_PER_SEC / 1000.0f);    \
            printf("%fms", MilliSeconds);                                           \
        }                                                                           \
        else {                                                                      \
            double Seconds = (double)duration / (double)CLOCKS_PER_SEC;             \
            printf("%fs", Seconds);                                                 \
        }                                                                           \
    } while(0)

#endif 
