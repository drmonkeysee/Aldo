//
//  cycleclock.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/15/22.
//

#ifndef Aldo_cycleclock_h
#define Aldo_cycleclock_h

#include <stdint.h>

struct cycleclock {
    uint64_t frames, total_cycles;
    double runtime;
    int budget, cycles_per_sec;
};

#endif
