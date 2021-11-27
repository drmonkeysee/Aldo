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

struct cycleclock {
    uint64_t total_cycles;
    int budget, cycles_per_sec;
};

struct control {
    struct cycleclock clock;
    const char *cartfile, *chrdecode_prefix, *me;
    int chrscale, ramsheet;
    bool chrdecode, disassemble, help, info, running, tron, verbose, version;
};

extern const int MinCps, MaxCps, RamSheets;

const char *ctrl_cartfilename(const char *cartfile);

#endif
