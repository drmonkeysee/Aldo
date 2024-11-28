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
#define ALDO_HEXPR_COND_X \
X(HLT_NONE, "None") \
X(HLT_ADDR, "Address") \
X(HLT_TIME, "Time") \
X(HLT_CYCLES, "Cycles") \
X(HLT_FRAMES, "Frames") \
X(HLT_JAM, "Jammed")

enum aldo_haltcondition {
#define X(s, d) ALDO_##s,
    ALDO_HEXPR_COND_X
#undef X
    ALDO_HLT_COUNT,
};

#define ALDO_HEXPR_RST_IND "!"

struct aldo_haltexpr {
    union {
        uint64_t cycles, frames;
        float runtime;
        uint16_t address;
    };
    enum aldo_haltcondition cond;
};

struct aldo_debugexpr {
    union {
        struct aldo_haltexpr hexpr;
        int resetvector;
    };
    enum {
        ALDO_DBG_EXPR_HALT,
        ALDO_DBG_EXPR_RESET,
    } type;
};

enum {
    ALDO_HEXPR_FMT_SIZE = 25,
};

// X(symbol, value, error string)
#define ALDO_HEXPR_ERRCODE_X \
X(HEXPR_ERR_SCAN, -1, "FORMATTED INPUT FAILURE") \
X(HEXPR_ERR_VALUE, -2, "INVALID PARSED VALUE") \
X(HEXPR_ERR_FMT, -3, "FORMATTED OUTPUT FAILURE") \
X(HEXPR_ERR_COND, -4, "INVALID HALT CONDITION")

enum {
#define X(s, v, e) ALDO_##s = v,
    ALDO_HEXPR_ERRCODE_X
#undef X
};

#include "bridgeopen.h"
br_libexport
const char *aldo_haltexpr_errstr(int err) br_nothrow;
br_libexport
const char *aldo_haltcond_description(enum aldo_haltcondition cond) br_nothrow;

// NOTE: if returns non-zero error code, *expr is unmodified
br_libexport br_checkerror
int aldo_haltexpr_parse(const char *br_noalias str,
                        struct aldo_haltexpr *expr) br_nothrow;
br_libexport br_checkerror
int aldo_haltexpr_parse_dbg(const char *br_noalias str,
                            struct aldo_debugexpr *expr) br_nothrow;
br_libexport br_checkerror
int aldo_haltexpr_desc(const struct aldo_haltexpr *expr,
                       char buf[br_nacsz(ALDO_HEXPR_FMT_SIZE)]) br_nothrow;
br_libexport br_checkerror
int aldo_haltexpr_fmtdbg(const struct aldo_debugexpr *expr,
                         char buf[br_nacsz(ALDO_HEXPR_FMT_SIZE)]) br_nothrow;
#include "bridgeclose.h"

#endif
