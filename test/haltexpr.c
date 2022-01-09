//
//  haltexpr.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 1/8/22.
//

#include "ciny.h"
#include "haltexpr.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static void null_string(void *ctx)
{
    const char *const str = NULL;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, NULL);

    ct_assertfalse(result);
}

static void empty_string(void *ctx)
{
    const char *const str = "";
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, NULL);

    ct_assertfalse(result);
}

static void empty_string_with_end(void *ctx)
{
    const char *const str = "", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_assertfalse(result);
    ct_assertsame(str, end);
}

static void address_condition(void *ctx)
{
    const char *const str = "@ab12", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_with_space(void *ctx)
{
    const char *const str = "@   ab12", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_with_leading_space(void *ctx)
{
    const char *const str = "   @ab12", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_with_trailing_space(void *ctx)
{
    const char *const str = "@ab12   ", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_caps(void *ctx)
{
    const char *const str = "@AB12", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_with_prefix(void *ctx)
{
    const char *const str = "@0xab12", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_short(void *ctx)
{
    const char *const str = "@1f", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0x001fu, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_trailing_exprs(void *ctx)
{
    const char *const str = "@ab12,foo,bar", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(strchr(str, 'f'), end);
    ct_assertequal('f', *end);
}

static void address_condition_trailing_exprs_spaced(void *ctx)
{
    const char *const str = "@ab12, foo,bar", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
    ct_assertsame(strchr(str, ' '), end);
    ct_assertequal(' ', *end);
}

static void address_condition_overparse(void *ctx)
{
    const char *const str = "@12345", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0x2345u, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_underparse(void *ctx)
{
    const char *const str = "@asdf", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xau, expr.address);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void address_condition_malformed(void *ctx)
{
    const char *const str = "@hjkl", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_assertfalse(result);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void runtime_condition(void *ctx)
{
    const char *const str = "1.2s", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void runtime_condition_with_space(void *ctx)
{
    const char *const str = "1.2   s", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void runtime_condition_with_leading_space(void *ctx)
{
    const char *const str = "   1.2s", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void runtime_condition_with_trailing_space(void *ctx)
{
    const char *const str = "1.2s   ", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void runtime_condition_no_decimal(void *ctx)
{
    const char *const str = "5s", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(5.0f, expr.runtime, 0.001);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void runtime_condition_trailing_exprs(void *ctx)
{
    const char *const str = "1.2s,foo,bar", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
    ct_assertsame(strchr(str, 'f'), end);
    ct_assertequal('f', *end);
}

static void runtime_condition_trailing_exprs_spaced(void *ctx)
{
    const char *const str = "1.2s, foo,bar", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
    ct_assertsame(strchr(str, ' '), end);
    ct_assertequal(' ', *end);
}

static void runtime_condition_malformed(void *ctx)
{
    const char *const str = "fasdf", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_assertfalse(result);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void cycles_condition(void *ctx)
{
    const char *const str = "42c", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void cycles_condition_with_space(void *ctx)
{
    const char *const str = "42   c", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void cycles_condition_with_leading_space(void *ctx)
{
    const char *const str = "   42c", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void cycles_condition_with_trailing_space(void *ctx)
{
    const char *const str = "42c   ", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void cycles_condition_trailing_exprs(void *ctx)
{
    const char *const str = "42c,foo,bar", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
    ct_assertsame(strchr(str, 'f'), end);
    ct_assertequal('f', *end);
}

static void cycles_condition_trailing_exprs_spaced(void *ctx)
{
    const char *const str = "42c, foo,bar", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_asserttrue(result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
    ct_assertsame(strchr(str, ' '), end);
    ct_assertequal(' ', *end);
}

static void cycles_condition_malformed(void *ctx)
{
    const char *const str = "fasdf", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_assertfalse(result);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

static void expr_missing_unit(void *ctx)
{
    const char *const str = "1234", *end;
    struct haltexpr expr;

    const bool result = haltexpr_parse(str, &expr, &end);

    ct_assertfalse(result);
    ct_assertsame(str + strlen(str), end);
    ct_assertequal('\0', *end);
}

//
// Test List
//

struct ct_testsuite haltexpr_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(null_string),
        ct_maketest(empty_string),
        ct_maketest(empty_string_with_end),

        ct_maketest(address_condition),
        ct_maketest(address_condition_with_space),
        ct_maketest(address_condition_with_leading_space),
        ct_maketest(address_condition_with_trailing_space),
        ct_maketest(address_condition_caps),
        ct_maketest(address_condition_with_prefix),
        ct_maketest(address_condition_short),
        ct_maketest(address_condition_with_prefix),
        ct_maketest(address_condition_trailing_exprs),
        ct_maketest(address_condition_trailing_exprs_spaced),
        ct_maketest(address_condition_overparse),
        ct_maketest(address_condition_underparse),
        ct_maketest(address_condition_malformed),

        ct_maketest(runtime_condition),
        ct_maketest(runtime_condition_with_space),
        ct_maketest(runtime_condition_with_leading_space),
        ct_maketest(runtime_condition_with_trailing_space),
        ct_maketest(runtime_condition_no_decimal),
        ct_maketest(runtime_condition_trailing_exprs),
        ct_maketest(runtime_condition_trailing_exprs_spaced),
        ct_maketest(runtime_condition_malformed),

        ct_maketest(cycles_condition),
        ct_maketest(cycles_condition_with_space),
        ct_maketest(cycles_condition_with_leading_space),
        ct_maketest(cycles_condition_with_trailing_space),
        ct_maketest(cycles_condition_trailing_exprs),
        ct_maketest(cycles_condition_trailing_exprs_spaced),
        ct_maketest(cycles_condition_malformed),

        ct_maketest(expr_missing_unit),
    };

    return ct_makesuite(tests);
}
