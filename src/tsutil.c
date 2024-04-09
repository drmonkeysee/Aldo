//
//  tsutil.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/14/22.
//

#include "tsutil.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>

extern inline double timespec_to_ms(const struct timespec *);

struct timespec timespec_elapsed(const struct timespec *from)
{
    assert(from != NULL);

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

void timespec_sleep(struct timespec duration)
{
    struct timespec ts_left;
    int result;
    do {
#ifdef __APPLE__
        result = nanosleep(&duration, &ts_left) ? errno : 0;
#else
        result = clock_nanosleep(CLOCK_MONOTONIC, 0, &duration, &ts_left);
#endif
        duration = ts_left;
    } while (result == EINTR);
}
