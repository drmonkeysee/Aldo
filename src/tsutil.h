//
//  tsutil.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/1/22.
//

#ifndef Aldo_tsutil_h
#define Aldo_tsutil_h

#include <time.h>

#define ALDO_MS_PER_S 1e3
#define ALDO_NS_PER_MS 1e6

#include "bridgeopen.h"
aldo_const int AldoNsPerS = (int)(ALDO_MS_PER_S * ALDO_NS_PER_MS);

aldo_export
inline double aldo_timespec_to_ms(const struct timespec *ts) aldo_nothrow
{
    return (ts->tv_sec * ALDO_MS_PER_S) + (ts->tv_nsec / ALDO_NS_PER_MS);
}

aldo_export
struct timespec aldo_elapsed(const struct timespec *from) aldo_nothrow;
aldo_export
void aldo_sleep(struct timespec duration) aldo_nothrow;
#include "bridgeclose.h"

#endif
