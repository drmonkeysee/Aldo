//
//  control.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/14/21.
//

#ifndef Aldo_control_h
#define Aldo_control_h

#include <stdbool.h>
#include <stdint.h>

enum excmode {
    EXC_CYCLE,
    EXC_STEP,
    EXC_RUN,
    EXC_MODECOUNT,
};

struct control {
    uint64_t total_cycles;
    enum excmode exec_mode;
    int cyclebudget, cycles_per_sec, ramsheet;
    bool running;
};

extern const int RamSheets;

#endif
