//
//  cycleclock.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/24/22.
//

#include "cycleclock.h"
#include "tsutil.h"

void cycleclock_start(struct cycleclock *self)
{
    clock_gettime(CLOCK_MONOTONIC, &self->start);
    self->previous = self->start;
}

void cycleclock_tickstart(struct cycleclock *self, bool reset_budget)
{
    clock_gettime(CLOCK_MONOTONIC, &self->current);
    const double currentms = timespec_to_ms(&self->current);
    self->frametime_ms = currentms - timespec_to_ms(&self->previous);
    self->runtime = (currentms - timespec_to_ms(&self->start)) / TSU_MS_PER_S;

    if (reset_budget) {
        self->timebudget_ms = self->budget = 0;
        return;
    }

    self->timebudget_ms += self->frametime_ms;
    // NOTE: accumulate at most a second of banked cycle time
    if (self->timebudget_ms >= TSU_MS_PER_S) {
        self->timebudget_ms = TSU_MS_PER_S;
    }

    const double mspercycle = (double)TSU_MS_PER_S / self->cycles_per_sec;
    const int new_cycles = (int)(self->timebudget_ms / mspercycle);
    self->budget += new_cycles;
    self->timebudget_ms -= new_cycles * mspercycle;
}

void cycleclock_tickend(struct cycleclock *self)
{
    self->previous = self->current;
    ++self->frames;
}
