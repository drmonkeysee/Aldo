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
    uint64_t cycles, frames, ticks;
    double emutime, runtime, ticktime_ms, timebudget_ms;
    int budget, cpf, fps;
    uint8_t subcycle;
};

#include "bridgeopen.h"
br_libexport
extern const int MinCpf, MaxCpf;

br_libexport
void cycleclock_start(struct cycleclock *self) br_nothrow;
br_libexport
void cycleclock_tickstart(struct cycleclock *self,
                          bool reset_budget) br_nothrow;
br_libexport
void cycleclock_tickend(struct cycleclock *self) br_nothrow;
#include "bridgeclose.h"

#endif
