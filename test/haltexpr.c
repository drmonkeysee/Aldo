//
//  haltexpr.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 1/8/22.
//

#include "ciny.h"
#include "haltexpr.h"

#include <stddef.h>
#include <string.h>

//
// Error Strings
//

static void errstr_returns_known_err(void *ctx)
{
    const char *const err = haltexpr_errstr(HEXPR_ERR_FMT);

    ct_assertequalstr("FORMATTED OUTPUT FAILURE", err);
}

static void errstr_returns_unknown_err(void *ctx)
{
    const char *const err = haltexpr_errstr(10);

    ct_assertequalstr("UNKNOWN ERR", err);
}

//
// Expression Parse
//

static void null_string(void *ctx)
{
    const char *const str = NULL;
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void empty_string(void *ctx)
{
    const char *const str = "";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void address_condition(void *ctx)
{
    const char *const str = "@ab12";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
}

static void address_condition_with_space(void *ctx)
{
    const char *const str = "@   ab12";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
}

static void address_condition_with_leading_space(void *ctx)
{
    const char *const str = "   @ab12";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
}

static void address_condition_with_trailing_space(void *ctx)
{
    const char *const str = "@ab12   ";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
}

static void address_condition_caps(void *ctx)
{
    const char *const str = "@AB12";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
}

static void address_condition_with_prefix(void *ctx)
{
    const char *const str = "@0xab12";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0xab12u, expr.address);
}

static void address_condition_short(void *ctx)
{
    const char *const str = "@1f";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_ADDR, (int)expr.cond);
    ct_assertequal(0x001fu, expr.address);
}

static void address_condition_too_large(void *ctx)
{
    const char *const str = "@12345";
    struct haltexpr expr = {0};

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_VALUE, result);
    ct_assertequal(HLT_NONE, (int)expr.cond);
}

static void address_condition_negative_overflow(void *ctx)
{
    const char *const str = "@-asdf";
    struct haltexpr expr = {0};

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_VALUE, result);
    ct_assertequal(HLT_NONE, (int)expr.cond);
}

static void address_condition_malformed(void *ctx)
{
    const char *const str = "@hjkl";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void runtime_condition(void *ctx)
{
    const char *const str = "1.2s";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
}

static void runtime_condition_case_insensitive(void *ctx)
{
    const char *const str = "1.2S";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
}

static void runtime_condition_with_space(void *ctx)
{
    const char *const str = "1.2   s";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
}

static void runtime_condition_with_leading_space(void *ctx)
{
    const char *const str = "   1.2s";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
}

static void runtime_condition_with_trailing_space(void *ctx)
{
    const char *const str = "1.2s   ";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(1.2f, expr.runtime, 0.001);
}

static void runtime_condition_no_decimal(void *ctx)
{
    const char *const str = "5s";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(5.0f, expr.runtime, 0.001);
}

static void runtime_condition_negative(void *ctx)
{
    const char *const str = "-20.4s";
    struct haltexpr expr = {0};

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_VALUE, result);
    ct_assertequal(HLT_NONE, (int)expr.cond);
}

static void runtime_condition_malformed(void *ctx)
{
    const char *const str = "hjkls";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void cycles_condition(void *ctx)
{
    const char *const str = "42c";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
}

static void cycles_condition_case_insensitive(void *ctx)
{
    const char *const str = "42C";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
}

static void cycles_condition_with_space(void *ctx)
{
    const char *const str = "42   c";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
}

static void cycles_condition_with_leading_space(void *ctx)
{
    const char *const str = "   42c";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
}

static void cycles_condition_with_trailing_space(void *ctx)
{
    const char *const str = "42c   ";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(42u, expr.cycles);
}

static void cycles_condition_negative(void *ctx)
{
    const char *const str = "-45c";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_CYCLES, (int)expr.cond);
    ct_assertequal(18446744073709551571u, expr.cycles);
}

static void cycles_condition_malformed(void *ctx)
{
    const char *const str = "hjklc";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void jam_condition(void *ctx)
{
    const char *const str = "jam";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_JAM, (int)expr.cond);
}

static void jam_condition_uppercase(void *ctx)
{
    const char *const str = "JAM";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_JAM, (int)expr.cond);
}

static void jam_condition_mixedcase(void *ctx)
{
    const char *const str = "JaM";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_JAM, (int)expr.cond);
}

static void jam_condition_with_leading_space(void *ctx)
{
    const char *const str = "   jam";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_JAM, (int)expr.cond);
}

static void jam_condition_with_trailing_space(void *ctx)
{
    const char *const str = "jam   ";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_JAM, (int)expr.cond);
}

static void jam_condition_underparse(void *ctx)
{
    const char *const str = "jamming";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_JAM, (int)expr.cond);
}

static void jam_condition_malformed(void *ctx)
{
    const char *const str = "maj";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void expr_missing_unit(void *ctx)
{
    const char *const str = "1234";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

//
// Expression Print
//

static void print_none(void *ctx)
{
    struct haltexpr expr = {0};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt(&expr, buf);

    const char *const expected = "None";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_addr(void *ctx)
{
    struct haltexpr expr = {.cond = HLT_ADDR, .address = 0x1234};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt(&expr, buf);

    const char *const expected = "@ $1234";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_runtime(void *ctx)
{
    struct haltexpr expr = {.cond = HLT_TIME, .runtime = 4.36532245f};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt(&expr, buf);

    const char *const expected = "4.365 sec";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_cycles(void *ctx)
{
    struct haltexpr expr = {.cond = HLT_CYCLES, .cycles = 982423};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt(&expr, buf);

    const char *const expected = "982423 cyc";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_jam(void *ctx)
{
    struct haltexpr expr = {.cond = HLT_JAM};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt(&expr, buf);

    const char *const expected = "JAM";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

//
// Test List
//

struct ct_testsuite haltexpr_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(errstr_returns_known_err),
        ct_maketest(errstr_returns_unknown_err),

        ct_maketest(null_string),
        ct_maketest(empty_string),

        ct_maketest(address_condition),
        ct_maketest(address_condition_with_space),
        ct_maketest(address_condition_with_leading_space),
        ct_maketest(address_condition_with_trailing_space),
        ct_maketest(address_condition_caps),
        ct_maketest(address_condition_with_prefix),
        ct_maketest(address_condition_short),
        ct_maketest(address_condition_with_prefix),
        ct_maketest(address_condition_too_large),
        ct_maketest(address_condition_negative_overflow),
        ct_maketest(address_condition_malformed),

        ct_maketest(runtime_condition),
        ct_maketest(runtime_condition_case_insensitive),
        ct_maketest(runtime_condition_with_space),
        ct_maketest(runtime_condition_with_leading_space),
        ct_maketest(runtime_condition_with_trailing_space),
        ct_maketest(runtime_condition_no_decimal),
        ct_maketest(runtime_condition_negative),
        ct_maketest(runtime_condition_malformed),

        ct_maketest(cycles_condition),
        ct_maketest(cycles_condition_case_insensitive),
        ct_maketest(cycles_condition_with_space),
        ct_maketest(cycles_condition_with_leading_space),
        ct_maketest(cycles_condition_with_trailing_space),
        ct_maketest(cycles_condition_negative),
        ct_maketest(cycles_condition_malformed),

        ct_maketest(jam_condition),
        ct_maketest(jam_condition_uppercase),
        ct_maketest(jam_condition_mixedcase),
        ct_maketest(jam_condition_with_leading_space),
        ct_maketest(jam_condition_with_trailing_space),
        ct_maketest(jam_condition_underparse),
        ct_maketest(jam_condition_malformed),

        ct_maketest(expr_missing_unit),

        ct_maketest(print_none),
        ct_maketest(print_addr),
        ct_maketest(print_runtime),
        ct_maketest(print_cycles),
        ct_maketest(print_jam),
    };

    return ct_makesuite(tests);
}
