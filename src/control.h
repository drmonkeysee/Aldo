//
//  control.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/14/21.
//

#ifndef Aldo_control_h
#define Aldo_control_h

#include <stdbool.h>

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
    struct bounce bouncer;
    int chrscale, resetvector;
    bool
        batch, bcdsupport, chrdecode, disassemble, help, info, tron,
        unified_disoutput, verbose, version, zeroram;
};

#endif
