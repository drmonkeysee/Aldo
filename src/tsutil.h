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

#include "bridgeopen.h"
br_libexport
inline double timespec_to_ms(const struct timespec *ts)
{
    return (ts->tv_sec * TSU_MS_PER_S) + (ts->tv_nsec / (double)TSU_NS_PER_MS);
}

br_libexport
struct timespec timespec_elapsed(const struct timespec *from);
br_libexport
void timespec_sleep(struct timespec duration);
#include "bridgeclose.h"

#endif
