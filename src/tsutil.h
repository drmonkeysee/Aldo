//
//  tsutil.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/1/22.
//

#ifndef Aldo_tsutil_h
#define Aldo_tsutil_h

#include <time.h>

enum {
    TSU_MS_PER_S = 1000,
    TSU_NS_PER_MS = (int)1e6,
    TSU_NS_PER_S = TSU_MS_PER_S * TSU_NS_PER_MS,
};

inline double timespec_to_ms(const struct timespec *ts)
{
    return (ts->tv_sec * TSU_MS_PER_S) + (ts->tv_nsec / (double)TSU_NS_PER_MS);
}

inline struct timespec timespec_elapsed(const struct timespec *from)
{
    struct timespec now, elapsed;
    clock_gettime(CLOCK_MONOTONIC, &now);

    elapsed = (struct timespec){.tv_sec = now.tv_sec - from->tv_sec};

    if (from->tv_nsec > now.tv_nsec) {
        // NOTE: subtract with borrow
        --elapsed.tv_sec;
        elapsed.tv_nsec = TSU_NS_PER_S - (from->tv_nsec - now.tv_nsec);
    } else {
        elapsed.tv_nsec = now.tv_nsec - from->tv_nsec;
    }
    return elapsed;
}

#endif
