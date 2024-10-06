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

static int parse_resetvector(const char *restrict str, int *resetvector)
{
    if (!str) return ALDO_HEXPR_ERR_SCAN;

    char u[2];
    unsigned int addr;
    bool
        parsed = sscanf(str, " %1[" ALDO_HEXPR_RST_IND "]%X", u, &addr) == 2,
        valid = addr < ALDO_MEMBLOCK_64KB;
    if (parsed) {
        if (!valid) return ALDO_HEXPR_ERR_VALUE;
        *resetvector = (int)addr;
        return 0;
    }
    return ALDO_HEXPR_ERR_SCAN;
}

//
// MARK: - Public Interface
//

const char *aldo_haltexpr_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case ALDO_##s: return e;
        ALDO_HEXPR_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

const char *aldo_haltcond_description(enum aldo_haltcondition cond)
{
    switch (cond) {
#define X(s, d) case ALDO_##s: return d;
        ALDO_HEXPR_COND_X
#undef X
    default:
        return "INVALID CONDITION";
    }
}

int aldo_haltexpr_parse(const char *restrict str, struct aldo_haltexpr *expr)
{
    assert(expr != NULL);

    if (!str) return ALDO_HEXPR_ERR_SCAN;

    bool parsed = false, valid = false;
    char u[2];
    struct aldo_haltexpr e;
    for (int i = ALDO_HLT_NONE + 1; i < ALDO_HLT_COUNT; ++i) {
        switch (i) {
        case ALDO_HLT_ADDR:
            {
                unsigned int addr;
                parsed = sscanf(str, " %1[@]%X", u, &addr) == 2;
                valid = addr < ALDO_MEMBLOCK_64KB;
                e = (struct aldo_haltexpr){
                    .address = (uint16_t)addr,
                    .cond = (enum aldo_haltcondition)i,
                };
            }
            break;
        case ALDO_HLT_TIME:
            {
                float time;
                parsed = sscanf(str, "%f %1[Ss]", &time, u) == 2;
                valid = time >= 0;
                e = (struct aldo_haltexpr){
                    .runtime = time,
                    .cond = (enum aldo_haltcondition)i,
                };
            }
            break;
        case ALDO_HLT_CYCLES:
            {
                uint64_t cycles;
                parsed = sscanf(str, "%" SCNu64 " %1[Cc]", &cycles, u) == 2;
                valid = true;
                e = (struct aldo_haltexpr){
                    .cycles = cycles,
                    .cond = (enum aldo_haltcondition)i,
                };
            }
            break;
        case ALDO_HLT_JAM:
            parsed = sscanf(str, " %1[Jj]%1[Aa]%1[Mm]", u, u, u) == 3;
            valid = true;
            e = (struct aldo_haltexpr){.cond = (enum aldo_haltcondition)i};
            break;
        default:
            assert(((void)"INVALID HALT CONDITION", false));
            return ALDO_HEXPR_ERR_COND;
        }
        if (parsed) {
            if (!valid) return ALDO_HEXPR_ERR_VALUE;
            *expr = e;
            return 0;
        }
    }
    return ALDO_HEXPR_ERR_SCAN;
}

int aldo_haltexpr_parse_dbg(const char *restrict str,
                            struct aldo_debugexpr *expr)
{
    assert(expr != NULL);

    struct aldo_haltexpr hexpr;
    int err = aldo_haltexpr_parse(str, &hexpr);
    if (err == 0) {
        *expr = (struct aldo_debugexpr){
            .hexpr = hexpr,
            .type = ALDO_DBG_EXPR_HALT,
        };
    } else {
        int resetvector;
        err = parse_resetvector(str, &resetvector);
        if (err < 0) return err;
        *expr = (struct aldo_debugexpr){
            .resetvector = resetvector,
            .type = ALDO_DBG_EXPR_RESET,
        };
    }
    return 0;
}

int aldo_haltexpr_desc(const struct aldo_haltexpr *expr,
                       char buf[restrict static ALDO_HEXPR_FMT_SIZE])
{
    assert(expr != NULL);
    assert(buf != NULL);

    int count;
    switch (expr->cond) {
    case ALDO_HLT_NONE:
        count = sprintf(buf, "None");
        break;
    case ALDO_HLT_ADDR:
        count = sprintf(buf, "PC @ $%04X", expr->address);
        break;
    case ALDO_HLT_TIME:
        count = sprintf(buf, "%.8g sec", expr->runtime);
        break;
    case ALDO_HLT_CYCLES:
        count = sprintf(buf, "%" PRIu64 " cyc", expr->cycles);
        break;
    case ALDO_HLT_JAM:
        count = sprintf(buf, "CPU JAMMED");
        break;
    default:
        assert(((void)"INVALID HALT CONDITION", false));
        return ALDO_HEXPR_ERR_COND;
    }

    if (count < 0) return ALDO_HEXPR_ERR_FMT;

    assert(count < ALDO_HEXPR_FMT_SIZE);
    return count;
}

int aldo_haltexpr_fmtdbg(const struct aldo_debugexpr *expr,
                         char buf[restrict static ALDO_HEXPR_FMT_SIZE])
{
    assert(expr != NULL);
    assert(buf != NULL);

    int count;
    if (expr->type == ALDO_DBG_EXPR_RESET) {
        count = sprintf(buf, ALDO_HEXPR_RST_IND "%04X", expr->resetvector);
    } else {
        const struct aldo_haltexpr *hexpr = &expr->hexpr;
        switch (hexpr->cond) {
        case ALDO_HLT_ADDR:
            count = sprintf(buf, "@%04X", hexpr->address);
            break;
        case ALDO_HLT_TIME:
            count = sprintf(buf, "%.8gs", hexpr->runtime);
            break;
        case ALDO_HLT_CYCLES:
            count = sprintf(buf, "%" PRIu64 "c", hexpr->cycles);
            break;
        case ALDO_HLT_JAM:
            count = sprintf(buf, "JAM");
            break;
        default:
            assert(((void)"INVALID HALT CONDITION", false));
            return ALDO_HEXPR_ERR_COND;
        }
    }

    if (count < 0) return ALDO_HEXPR_ERR_FMT;

    assert(count < ALDO_HEXPR_FMT_SIZE);
    return count;
}

