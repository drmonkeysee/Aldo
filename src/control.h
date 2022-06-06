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
    double runtime;
    int budget, cycles_per_sec;
};

struct haltarg {
    const char *expr;       // Non-owning Pointer
    struct haltarg *next;
};

struct control {
    struct haltarg *haltlist;
    const char *cartfile, *chrdecode_prefix, *me;   // Non-owning Pointers
    struct cycleclock clock;
    int chrscale, ramsheet, resetvector;
    bool
        batch, bcdsupport, chrdecode, disassemble, help, info, running, tron,
        unified_disoutput, verbose, version;
};

extern const int MinChrScale, MaxChrScale, MinCps, MaxCps, RamSheets;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *ctrl_cartfilename(const char *cartfile);

#endif
