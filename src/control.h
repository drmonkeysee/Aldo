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

struct control {
    uint64_t total_cycles;
    int cyclebudget, cycles_per_sec, ramsheet;
    bool running;
};

extern const int RamSheets;

#endif
