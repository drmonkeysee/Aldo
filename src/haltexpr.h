//
//  haltexpr.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/7/22.
//

#ifndef Aldo_haltexpr_h
#define Aldo_haltexpr_h

#include <stdint.h>

// X(symbol, description)
#define HEXPR_COND_X \
X(HLT_NONE, "None") \
X(HLT_ADDR, "Address") \
X(HLT_TIME, "Time") \
X(HLT_CYCLES, "Cycles") \
X(HLT_JAM, "Jammed")

enum haltcondition {
#define X(s, d) s,
    HEXPR_COND_X
#undef X
    HLT_CONDCOUNT,
};

struct haltexpr {
    union {
        uint64_t cycles;
        float runtime;
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
br_libexport
extern const int NoResetVector;

br_libexport
const char *haltexpr_errstr(int err) br_nothrow;
br_libexport
const char *haltcond_description(enum haltcondition cond) br_nothrow;

// NOTE: if returns non-zero error code, *expr is unmodified
br_libexport br_checkerror
int haltexpr_parse(const char *br_noalias str,
                   struct haltexpr *expr) br_nothrow;
br_libexport br_checkerror
int haltexpr_parse_resetvector(const char *br_noalias str,
                               int *resetvector) br_nothrow;
br_libexport br_checkerror
int haltexpr_fmt(const struct haltexpr *expr,
                 char buf[br_noalias_csz(HEXPR_FMT_SIZE)]) br_nothrow;
#include "bridgeclose.h"

#endif
