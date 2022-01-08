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
#include <stdint.h>

// X(symbol, print format, scan format, scan format args)
#define HALT_EXPR_X \
X(HLT_ADDR, "@%04X", "%1[@]%" SCNx16, unit, &expr->address) \
X(HLT_TIME, "%fs", "%f%1[s]", &expr->runtime, unit) \
X(HLT_CYCLES, "%" PRIu64 "c", "%" SCNu64 "%1[c]", &expr->cycles, unit)

enum haltcondition {
#define X(s, pri, scn, ...) s,
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

bool haltexpr_parse(const char *str, struct haltexpr *expr, const char **end);

#endif
