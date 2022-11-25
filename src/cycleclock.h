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
    double frametime_ms, runtime, timebudget_ms;
    int budget, cycles_per_sec;
};

void cycleclock_start(struct cycleclock *self);
void cycleclock_tickstart(struct cycleclock *self, bool reset_budget);
void cycleclock_tickend(struct cycleclock *self);

#endif
