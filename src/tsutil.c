//
//  tsutil.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/14/22.
//

#include "tsutil.h"

#include <errno.h>

extern inline double timespec_to_ms(const struct timespec *);

struct timespec timespec_elapsed(const struct timespec *from)
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

void timespec_sleep(struct timespec duration)
{
    struct timespec ts_req;
    int result;
    do {
        ts_req = duration;
#ifdef __APPLE__
        errno = 0;
        result = nanosleep(&ts_req, &duration) ? errno : 0;
#else
        result = clock_nanosleep(CLOCK_MONOTONIC, 0, &tick_req, &tick_left);
#endif
    } while (result == EINTR);
}
