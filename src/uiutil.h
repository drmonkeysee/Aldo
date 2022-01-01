//
//  uiutil.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/1/22.
//

#ifndef Aldo_uiutil_h
#define Aldo_uiutil_h

#include <time.h>

enum {
    UITIL_MS_PER_S = 1000,
    UITIL_NS_PER_MS = (int)1e6,
    UITIL_NS_PER_S = UITIL_MS_PER_S * UITIL_NS_PER_MS,
};

inline double timespec_to_ms(const struct timespec *ts)
{
    return (ts->tv_sec * UITIL_MS_PER_S)
            + (ts->tv_nsec / (double)UITIL_NS_PER_MS);
}

inline struct timespec timespec_elapsed(const struct timespec *from)
{
    struct timespec now, elapsed;
    clock_gettime(CLOCK_MONOTONIC, &now);

    elapsed = (struct timespec){.tv_sec = now.tv_sec - from->tv_sec};

    if (from->tv_nsec > now.tv_nsec) {
        // NOTE: subtract with borrow
        --elapsed.tv_sec;
        elapsed.tv_nsec = UITIL_NS_PER_S - (from->tv_nsec - now.tv_nsec);
    } else {
        elapsed.tv_nsec = now.tv_nsec - from->tv_nsec;
    }
    return elapsed;
}

#endif
