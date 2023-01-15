//
//  haltexpr.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/7/22.
//

#include "haltexpr.h"

#include "bytes.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

const int NoResetVector = -1;

const char *haltexpr_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case s: return e;
        HEXPR_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

const char *haltcond_description(enum haltcondition cond)
{
    switch (cond) {
#define X(s, d) case s: return d;
        HEXPR_COND_X
#undef X
    default:
        return "INVALID CONDITION";
    }
}

int haltexpr_parse(const char *restrict str, struct haltexpr *expr)
{
    assert(expr != NULL);

    if (str == NULL) return HEXPR_ERR_SCAN;

    bool parsed = false, valid = false;
    char u[2];
    struct haltexpr e;
    for (int i = HLT_NONE + 1; i < HLT_CONDCOUNT; ++i) {
        switch (i) {
        case HLT_ADDR:
            {
                unsigned int addr;
                parsed = sscanf(str, " %1[@]%X", u, &addr) == 2;
                valid = addr < MEMBLOCK_64KB;
                e = (struct haltexpr){
                    .address = (uint16_t)addr,
                    .cond = (enum haltcondition)i,
                };
            }
            break;
        case HLT_TIME:
            {
                float time;
                parsed = sscanf(str, "%f %1[Ss]", &time, u) == 2;
                valid = time > 0.0;
                e = (struct haltexpr){
                    .runtime = time,
                    .cond = (enum haltcondition)i,
                };
            }
            break;
        case HLT_CYCLES:
            {
                uint64_t cycles;
                parsed = sscanf(str, "%" SCNu64 " %1[Cc]", &cycles, u) == 2;
                valid = true;
                e = (struct haltexpr){
                    .cycles = cycles,
                    .cond = (enum haltcondition)i,
                };
            }
            break;
        case HLT_JAM:
            parsed = sscanf(str, " %1[Jj]%1[Aa]%1[Mm]", u, u, u) == 3;
            valid = true;
            e = (struct haltexpr){.cond = (enum haltcondition)i};
            break;
        default:
            assert(((void)"INVALID HALT CONDITION", false));
            return HEXPR_ERR_COND;
        }
        if (parsed) {
            if (!valid) return HEXPR_ERR_VALUE;
            *expr = e;
            return 0;
        }
    }
    return HEXPR_ERR_SCAN;
}

int haltexpr_parse_resetvector(const char *restrict str, int *resetvector)
{
    assert(resetvector != NULL);

    *resetvector = NoResetVector;
    if (str == NULL) return HEXPR_ERR_SCAN;

    char u[2];
    unsigned int addr;
    const bool parsed = sscanf(str, " %1[!]%X", u, &addr) == 2,
                valid = addr < MEMBLOCK_64KB;
    if (parsed) {
        if (!valid) return HEXPR_ERR_VALUE;
        *resetvector = (int)addr;
        return 0;
    }
    return HEXPR_ERR_SCAN;
}

int haltexpr_fmt(const struct haltexpr *expr,
                 char buf[restrict static HEXPR_FMT_SIZE])
{
    assert(expr != NULL);
    assert(buf != NULL);

    int count;
    switch (expr->cond) {
    case HLT_NONE:
        count = sprintf(buf, "None");
        break;
    case HLT_ADDR:
        count = sprintf(buf, "PC @ $%04X", expr->address);
        break;
    case HLT_TIME:
        count = sprintf(buf, "%.8g sec", expr->runtime);
        break;
    case HLT_CYCLES:
        count = sprintf(buf, "%" PRIu64 " cyc", expr->cycles);
        break;
    case HLT_JAM:
        count = sprintf(buf, "CPU JAMMED");
        break;
    default:
        assert(((void)"INVALID HALT CONDITION", false));
        return HEXPR_ERR_COND;
    }

    if (count < 0) return HEXPR_ERR_FMT;

    assert(count < HEXPR_FMT_SIZE);
    return count;
}
