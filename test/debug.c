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
    *ctx = aldo_debug_new();
}

static void teardown(void **ctx)
{
    aldo_debug_free(*ctx);
}

static void verify_haltexpr(const struct aldo_haltexpr *expr,
                            const struct aldo_breakpoint *bp)
{
    ct_assertequal(expr->cond, bp->expr.cond);
    switch (bp->expr.cond) {
    case ALDO_HLT_ADDR:
        ct_assertequal(expr->address, bp->expr.address);
        break;
    case ALDO_HLT_TIME:
        ct_assertequal(expr->runtime, bp->expr.runtime);
        break;
    case ALDO_HLT_CYCLES:
        ct_assertequal(expr->cycles, bp->expr.cycles);
        break;
    default:
        ct_assertfail("Unexpected halt expr condition");
        break;
    }
}

static void new_debugger(void *ctx)
{
    aldo_debugger *dbg = ctx;

    ct_assertequal(Aldo_NoResetVector, aldo_debug_vector_override(dbg));
    ct_assertequal(0u, aldo_debug_bp_count(dbg));
    ct_assertnull(aldo_debug_bp_at(dbg, 1));
}

static void set_reset_vector(void *ctx)
{
    aldo_debugger *dbg = ctx;

    aldo_debug_set_vector_override(dbg, 0x1234);

    ct_assertequal(0x1234, aldo_debug_vector_override(dbg));
}

static void negative_bp_index(void *ctx)
{
    aldo_debugger *dbg = ctx;

    ct_assertnull(aldo_debug_bp_at(dbg, -1));
}

static void add_breakpoint(void *ctx)
{
    aldo_debugger *dbg = ctx;
    struct aldo_haltexpr expr = {.cond = ALDO_HLT_ADDR, .address = 0x4321};

    (void)aldo_debug_bp_add(dbg, expr);

    ct_assertequal(1u, aldo_debug_bp_count(dbg));
    const struct aldo_breakpoint *bp = aldo_debug_bp_at(dbg, 0);
    ct_assertnotnull(bp);
    ct_asserttrue(bp->enabled);
    ct_assertequal(expr.cond, bp->expr.cond);
    ct_assertequal(expr.address, bp->expr.address);
}

static void enable_disable_breakpoint(void *ctx)
{
    aldo_debugger *dbg = ctx;
    struct aldo_haltexpr expr = {.cond = ALDO_HLT_ADDR, .address = 0x4321};

    (void)aldo_debug_bp_add(dbg, expr);
    aldo_debug_bp_enable(dbg, 0, false);

    ct_assertequal(1u, aldo_debug_bp_count(dbg));

    const struct aldo_breakpoint *bp = aldo_debug_bp_at(dbg, 0);
    ct_assertnotnull(bp);
    ct_assertfalse(bp->enabled);

    aldo_debug_bp_enable(dbg, 0, true);
    ct_asserttrue(bp->enabled);
}

static void multiple_breakpoints(void *ctx)
{
    aldo_debugger *dbg = ctx;
    struct aldo_haltexpr exprs[] = {
        {.cond = ALDO_HLT_ADDR, .address = 0x4321},
        {.cond = ALDO_HLT_TIME, .runtime = 34.5},
        {.cond = ALDO_HLT_CYCLES, .cycles = 3000},
    };
    size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        (void)aldo_debug_bp_add(dbg, exprs[i]);
    }

    ct_assertequal(3u, aldo_debug_bp_count(dbg));
    for (size_t i = 0; i < len; ++i) {
        const struct aldo_breakpoint *bp = aldo_debug_bp_at(dbg, (ptrdiff_t)i);
        ct_assertnotnull(bp);
        ct_asserttrue(bp->enabled);
        verify_haltexpr(exprs + i, bp);
    }
}

static void out_of_range(void *ctx)
{
    aldo_debugger *dbg = ctx;
    struct aldo_haltexpr exprs[] = {
        {.cond = ALDO_HLT_ADDR, .address = 0x4321},
        {.cond = ALDO_HLT_TIME, .runtime = 34.5},
        {.cond = ALDO_HLT_CYCLES, .cycles = 3000},
    };
    size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        (void)aldo_debug_bp_add(dbg, exprs[i]);
    }

    ct_assertequal(3u, aldo_debug_bp_count(dbg));
    ct_assertnull(aldo_debug_bp_at(dbg, 5));
}

static void delete_breakpoint(void *ctx)
{
    aldo_debugger *dbg = ctx;
    struct aldo_haltexpr exprs[] = {
        {.cond = ALDO_HLT_ADDR, .address = 0x4321},
        {.cond = ALDO_HLT_TIME, .runtime = 34.5},
        {.cond = ALDO_HLT_CYCLES, .cycles = 3000},
    };
    size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        (void)aldo_debug_bp_add(dbg, exprs[i]);
    }

    ct_assertequal(3u, aldo_debug_bp_count(dbg));

    aldo_debug_bp_remove(dbg, 1);
    ct_assertequal(2u, aldo_debug_bp_count(dbg));

    const struct aldo_breakpoint *bp = aldo_debug_bp_at(dbg, 0);
    ct_assertnotnull(bp);
    ct_asserttrue(bp->enabled);
    verify_haltexpr(exprs, bp);

    bp = aldo_debug_bp_at(dbg, 1);
    ct_assertnotnull(bp);
    ct_asserttrue(bp->enabled);
    verify_haltexpr(exprs + 2, bp);
}

static void clear_breakpoints(void *ctx)
{
    aldo_debugger *dbg = ctx;
    struct aldo_haltexpr exprs[] = {
        {.cond = ALDO_HLT_ADDR, .address = 0x4321},
        {.cond = ALDO_HLT_TIME, .runtime = 34.5},
        {.cond = ALDO_HLT_CYCLES, .cycles = 3000},
    };
    size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        (void)aldo_debug_bp_add(dbg, exprs[i]);
    }
    aldo_debug_set_vector_override(dbg, 0x1234);

    ct_assertequal(3u, aldo_debug_bp_count(dbg));
    ct_assertequal(0x1234, aldo_debug_vector_override(dbg));

    aldo_debug_bp_clear(dbg);

    ct_assertequal(0u, aldo_debug_bp_count(dbg));
    ct_assertequal(0x1234, aldo_debug_vector_override(dbg));
}

static void halt_no_breakpoint(void *ctx)
{
    aldo_debugger *dbg = ctx;

    ct_assertnull(aldo_debug_halted(dbg));
    ct_assertequal(Aldo_NoBreakpoint, aldo_debug_halted_at(dbg));
}

static void reset_debugger(void *ctx)
{
    aldo_debugger *dbg = ctx;
    struct aldo_haltexpr exprs[] = {
        {.cond = ALDO_HLT_ADDR, .address = 0x4321},
        {.cond = ALDO_HLT_TIME, .runtime = 34.5},
        {.cond = ALDO_HLT_CYCLES, .cycles = 3000},
    };
    size_t len = sizeof exprs / sizeof exprs[0];

    for (size_t i = 0; i < len; ++i) {
        (void)aldo_debug_bp_add(dbg, exprs[i]);
    }
    aldo_debug_set_vector_override(dbg, 0x1234);

    ct_assertequal(3u, aldo_debug_bp_count(dbg));
    ct_assertequal(0x1234, aldo_debug_vector_override(dbg));

    aldo_debug_reset(dbg);

    ct_assertequal(0u, aldo_debug_bp_count(dbg));
    ct_assertequal(Aldo_NoResetVector, aldo_debug_vector_override(dbg));
}

//
// MARK: - Test List
//

struct ct_testsuite debug_tests()
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
        ct_maketest(halt_no_breakpoint),

        ct_maketest(reset_debugger),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
