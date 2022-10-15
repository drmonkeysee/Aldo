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

#include "interopopen.h"

struct cycleclock {
    uint64_t frames, total_cycles;
    double runtime;
    int budget, cycles_per_sec;
};

struct haltarg {
    const char *expr;       // Non-owning Pointer
    struct haltarg *next;
};

// TODO: temp struct for gui demo
struct bounce {
    struct {
        int x, y;
    } bounds, pos, velocity;
    int dim;
};

struct control {
    struct haltarg *haltlist;
    const char *cartfile, *chrdecode_prefix, *me;   // Non-owning Pointers
    struct cycleclock clock;
    struct bounce bouncer;
    int chrscale, resetvector;
    bool
        batch, bcdsupport, chrdecode, disassemble, help, info, tron,
        unified_disoutput, verbose, version, zeroram;
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *ctrl_cartfilename(const char *cartfile) aldo_noexcept;

#include "interopclose.h"
#endif
