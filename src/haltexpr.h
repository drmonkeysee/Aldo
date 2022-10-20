//
//  haltexpr.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/7/22.
//

#ifndef Aldo_haltexpr_h
#define Aldo_haltexpr_h

#include <stdint.h>

enum haltcondition {
    HLT_NONE,
    HLT_ADDR,
    HLT_TIME,
    HLT_CYCLES,
    HLT_JAM,
    HLT_CONDCOUNT,
};

struct haltexpr {
    union {
        uint64_t cycles;
        double runtime;
        uint16_t address;
    };
    enum haltcondition cond;
};

enum {
    HEXPR_FMT_SIZE = 25,    // Halt expr description is at most 24 chars
};

// X(symbol, value, error string)
#define HEXPR_ERRCODE_X \
X(HEXPR_ERR_SCAN, -1, "FORMATTED INPUT FAILURE") \
X(HEXPR_ERR_VALUE, -2, "INVALID PARSED VALUE") \
X(HEXPR_ERR_FMT, -3, "FORMATTED OUTPUT FAILURE") \
X(HEXPR_ERR_COND, -4, "INVALID HALT CONDITION")

enum {
#define X(s, v, e) s = v,
    HEXPR_ERRCODE_X
#undef X
};

#include "bridgeopen.h"
// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *haltexpr_errstr(int err) br_nothrow;

// NOTE: if returns non-zero error code, *expr is unmodified
int haltexpr_parse(const char *br_noalias str,
                   struct haltexpr *expr) br_nothrow;
int haltexpr_fmt(const struct haltexpr *expr,
                 char buf[br_noalias_csz(HEXPR_FMT_SIZE)]) br_nothrow;
#include "bridgeclose.h"

#endif
