//
//  cycleclock.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/15/22.
//

#ifndef Aldo_cycleclock_h
#define Aldo_cycleclock_h

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

struct cycleclock {
    struct timespec current, previous, start;
    uint64_t frames, total_cycles;
    double emutime, frametime_ms, runtime, timebudget_ms;
    int budget, cycles_per_sec;
};

#include "bridgeopen.h"
br_libexport
extern const int MinCps, MaxCps;

br_libexport
void cycleclock_start(struct cycleclock *self) br_nothrow;
br_libexport
void cycleclock_tickstart(struct cycleclock *self,
                          bool reset_budget) br_nothrow;
br_libexport
void cycleclock_tickend(struct cycleclock *self) br_nothrow;
#include "bridgeclose.h"

#endif
