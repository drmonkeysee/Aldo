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
    ALDO_MS_PER_S = 1000,
    ALDO_NS_PER_MS = (int)1e6,
    ALDO_NS_PER_S = ALDO_MS_PER_S * ALDO_NS_PER_MS,
};

#include "bridgeopen.h"
br_libexport
inline double aldo_timespec_to_ms(const struct timespec *ts) br_nothrow
{
    return (double)(ts->tv_sec * ALDO_MS_PER_S)
                    + ((double)ts->tv_nsec / (double)ALDO_NS_PER_MS);
}

br_libexport
struct timespec aldo_elapsed(const struct timespec *from) br_nothrow;
br_libexport
void aldo_sleep(struct timespec duration) br_nothrow;
#include "bridgeclose.h"

#endif
