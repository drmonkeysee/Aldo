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
    int ramsheet;
    bool running;
};

extern const int RamSheets;

inline void ctl_toggle_excmode(struct control *c)
{
    c->exec_mode = (c->exec_mode + 1) % EXC_MODECOUNT;
}

inline void ctl_ram_next(struct control *c)
{
    c->ramsheet = (c->ramsheet + 1) % RamSheets;
}

inline void ctl_ram_prev(struct control *c)
{
    --c->ramsheet;
    if (c->ramsheet < 0) {
        c->ramsheet = RamSheets - 1;
    }
}

#endif
