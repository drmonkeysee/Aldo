//
//  cycleclock.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/15/22.
//

#ifndef Aldo_cycleclock_h
#define Aldo_cycleclock_h

#include <stdint.h>
#include <time.h>

enum aldo_clockscale {
    ALDO_CS_CYCLE,
    ALDO_CS_FRAME,
};

struct aldo_clock {
    struct timespec current, previous, start;
    uint64_t cycles, frames, ticks;
    double emutime, runtime, ticktime_ms, timebudget_ms;
    int budget, rate, rate_factor;
    uint8_t subcycle;
};

#include "bridgeopen.h"
aldo_export
extern const int Aldo_MinCps, Aldo_MaxCps, Aldo_MinFps, Aldo_MaxFps;

aldo_export
void aldo_clock_start(struct aldo_clock *self) aldo_nothrow;
aldo_export
void aldo_clock_tickstart(struct aldo_clock *self,
                          bool reset_budget) aldo_nothrow;
aldo_export
void aldo_clock_tickend(struct aldo_clock *self) aldo_nothrow;
#include "bridgeclose.h"

#endif
