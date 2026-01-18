//
//  cycleclock.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/24/22.
//

#include "cycleclock.h"
#include "tsutil.h"

const int
    Aldo_MinCps = 1, Aldo_MaxCps = 1000, Aldo_MinFps = 1, Aldo_MaxFps = 60;

void aldo_clock_start(struct aldo_clock *self)
{
    clock_gettime(CLOCK_MONOTONIC, &self->start);
    self->previous = self->start;
}

void aldo_clock_tickstart(struct aldo_clock *self, bool reset_budget)
{
    clock_gettime(CLOCK_MONOTONIC, &self->current);
    auto currentms = aldo_timespec_to_ms(&self->current);
    self->ticktime_ms = currentms - aldo_timespec_to_ms(&self->previous);
    self->runtime = (currentms - aldo_timespec_to_ms(&self->start))
                    / ALDO_MS_PER_S;

    if (reset_budget) {
        self->timebudget_ms = self->budget = 0;
        return;
    }

    self->emutime += self->ticktime_ms / ALDO_MS_PER_S;
    self->timebudget_ms += self->ticktime_ms;
    // NOTE: accumulate at most a second of banked cycle time
    if (self->timebudget_ms >= ALDO_MS_PER_S) {
        self->timebudget_ms = ALDO_MS_PER_S;
    }

    auto cycles_per_ms = (self->rate * self->rate_factor) / ALDO_MS_PER_S;
    auto new_cycles = (int)(self->timebudget_ms * cycles_per_ms);
    self->budget += new_cycles;
    self->timebudget_ms -= new_cycles / cycles_per_ms;
}

void aldo_clock_tickend(struct aldo_clock *self)
{
    self->previous = self->current;
    ++self->ticks;
}
