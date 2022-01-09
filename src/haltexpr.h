//
//  haltexpr.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/7/22.
//

#ifndef Aldo_haltexpr_h
#define Aldo_haltexpr_h

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// X(symbol, print format, print format arg, scan format, scan format args...)
#define HALT_EXPR_X \
X(HLT_ADDR, \
    "@ $%04X", expr->address, \
    " %1[@]%" SCNx16, u, &e.address) \
X(HLT_TIME, \
    "%.3f sec", expr->runtime, \
    "%f %1[s]", &e.runtime, u) \
X(HLT_CYCLES, \
    "%" PRIu64 " cyc", expr->cycles, \
    "%" SCNu64 " %1[c]", &e.cycles, u)

enum haltcondition {
    HLT_NONE,
#define X(s, pri, pa, scn, ...) s,
    HALT_EXPR_X
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

// NOTE: if return value is false, *expr is unmodified and *end is set to
// the end of the expression that failed to parse.
bool haltexpr_parse(const char *str, struct haltexpr *expr, const char **end);

// NOTE: if sz = 0, behaves like snprintf and buf is allowed to be NULL
int haltexpr_fmt(const struct haltexpr *expr, size_t sz,
                 char buf[restrict sz]);

#endif
