//
//  debug.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/18/23.
//

#include "ciny.h"
#include "debug.h"
#include "haltexpr.h"

#include <stddef.h>

static void setup(void **ctx)
{
    *ctx = debug_new();
}

static void teardown(void **ctx)
{
    debug_free(*ctx);
}

static void verify_haltexpr(const struct haltexpr *expr,
                            const struct breakpoint *bp)
{
    ct_assertequal(expr->cond, bp->expr.cond);
    switch (bp->expr.cond) {
    case HLT_ADDR:
        ct_assertequal(expr->address, bp->expr.address);
        break;
    case HLT_TIME:
        ct_assertequal(expr->runtime, bp->expr.runtime);
        break;
    case HLT_CYCLES:
        ct_assertequal(expr->cycles, bp->expr.cycles);
        break;
    default:
        ct_assertfail("Unexpected halt expr condition");
        break;
    }
}

static void new_debugger(void *ctx)
{
    debugger *const dbg = ctx;

    ct_assertequal(NoResetVector, debug_vector_override(dbg));
    ct_assertequal(0u, debug_bp_count(dbg));
    ct_assertnull(debug_bp_at(dbg, 1));
}

static void set_reset_vector(void *ctx)
{
    debugger *const dbg = ctx;

    debug_set_vector_override(dbg, 0x1234);

    ct_assertequal(0x1234, debug_vector_override(dbg));
}

static void negative_bp_index(void *ctx)
{
    debugger *const dbg = ctx;

    ct_assertnull(debug_bp_at(dbg, -1));
}

static void add_breakpoint(void *ctx)
{
    debugger *const dbg = ctx;
    const struct haltexpr expr = {.cond = HLT_ADDR, .address = 0x4321};

    debug_bp_add(dbg, expr);

    ct_assertequal(1u, debug_bp_count(dbg));
    const struct breakpoint *const bp = debug_bp_at(dbg, 0);
    ct_assertnotnull(bp);
    ct_asserttrue(bp->enabled);
    ct_assertequal(expr.cond, bp->expr.cond);
    ct_assertequal(expr.address, bp->expr.address);
}

static void enable_disable_breakpoint(void *ctx)
{
    debugger *const dbg = ctx;
    const struct haltexpr expr = {.cond = HLT_ADDR, .address = 0x4321};

    debug_bp_add(dbg, expr);
    debug_bp_enabled(dbg, 0, false);

    ct_assertequal(1u, debug_bp_count(dbg));

    const struct breakpoint *bp = debug_bp_at(dbg, 0);
    ct_assertnotnull(bp);
    ct_assertfalse(bp->enabled);

    debug_bp_enabled(dbg, 0, true);
    ct_asserttrue(bp->enabled);
}

static void multiple_breakpoints(void *ctx)
{
    debugger *const dbg = ctx;
    const struct haltexpr exprs[] = {
        {.cond = HLT_ADDR, .address = 0x4321},
        {.cond = HLT_TIME, .runtime = 34.5},
        {.cond = HLT_CYCLES, .cycles = 3000},
    };
    const size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        debug_bp_add(dbg, exprs[i]);
    }

    ct_assertequal(3u, debug_bp_count(dbg));
    for (size_t i = 0; i < len; ++i) {
        const struct breakpoint *const bp = debug_bp_at(dbg, (ptrdiff_t)i);
        ct_assertnotnull(bp);
        ct_asserttrue(bp->enabled);
        verify_haltexpr(exprs + i, bp);
    }
}

static void out_of_range(void *ctx)
{
    debugger *const dbg = ctx;
    const struct haltexpr exprs[] = {
        {.cond = HLT_ADDR, .address = 0x4321},
        {.cond = HLT_TIME, .runtime = 34.5},
        {.cond = HLT_CYCLES, .cycles = 3000},
    };
    const size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        debug_bp_add(dbg, exprs[i]);
    }

    ct_assertequal(3u, debug_bp_count(dbg));
    ct_assertnull(debug_bp_at(dbg, 5));
}

static void delete_breakpoint(void *ctx)
{
    debugger *const dbg = ctx;
    const struct haltexpr exprs[] = {
        {.cond = HLT_ADDR, .address = 0x4321},
        {.cond = HLT_TIME, .runtime = 34.5},
        {.cond = HLT_CYCLES, .cycles = 3000},
    };
    const size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        debug_bp_add(dbg, exprs[i]);
    }

    ct_assertequal(3u, debug_bp_count(dbg));

    debug_bp_remove(dbg, 1);
    ct_assertequal(2u, debug_bp_count(dbg));

    const struct breakpoint *bp = debug_bp_at(dbg, 0);
    ct_assertnotnull(bp);
    ct_asserttrue(bp->enabled);
    verify_haltexpr(exprs, bp);

    bp = debug_bp_at(dbg, 1);
    ct_assertnotnull(bp);
    ct_asserttrue(bp->enabled);
    verify_haltexpr(exprs + 2, bp);
}

static void clear_breakpoints(void *ctx)
{
    debugger *const dbg = ctx;
    const struct haltexpr exprs[] = {
        {.cond = HLT_ADDR, .address = 0x4321},
        {.cond = HLT_TIME, .runtime = 34.5},
        {.cond = HLT_CYCLES, .cycles = 3000},
    };
    const size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        debug_bp_add(dbg, exprs[i]);
    }
    debug_set_vector_override(dbg, 0x1234);

    ct_assertequal(3u, debug_bp_count(dbg));
    ct_assertequal(0x1234, debug_vector_override(dbg));

    debug_bp_clear(dbg);

    ct_assertequal(0u, debug_bp_count(dbg));
    ct_assertequal(0x1234, debug_vector_override(dbg));
}

static void reset_debugger(void *ctx)
{
    debugger *const dbg = ctx;
    const struct haltexpr exprs[] = {
        {.cond = HLT_ADDR, .address = 0x4321},
        {.cond = HLT_TIME, .runtime = 34.5},
        {.cond = HLT_CYCLES, .cycles = 3000},
    };
    const size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        debug_bp_add(dbg, exprs[i]);
    }
    debug_set_vector_override(dbg, 0x1234);

    ct_assertequal(3u, debug_bp_count(dbg));
    ct_assertequal(0x1234, debug_vector_override(dbg));

    debug_reset(dbg);

    ct_assertequal(0u, debug_bp_count(dbg));
    ct_assertequal(NoResetVector, debug_vector_override(dbg));
}

//
// MARK: - Test List
//

struct ct_testsuite debug_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(new_debugger),
        ct_maketest(set_reset_vector),

        ct_maketest(negative_bp_index),
        ct_maketest(add_breakpoint),
        ct_maketest(enable_disable_breakpoint),
        ct_maketest(multiple_breakpoints),
        ct_maketest(out_of_range),
        ct_maketest(delete_breakpoint),
        ct_maketest(clear_breakpoints),

        ct_maketest(reset_debugger),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
