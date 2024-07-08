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

#pragma mark - Error Strings
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

#pragma mark - Expression Parse
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

static void runtime_condition_zero(void *ctx)
{
    const char *const str = "0s";
    struct haltexpr expr;

    const int result = haltexpr_parse(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(HLT_TIME, (int)expr.cond);
    ct_assertaboutequal(0.0f, expr.runtime, 0.001);
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

#pragma mark - Reset Vector Parse
//
// Reset Vector Parse
//

static void null_resetvector_string(void *ctx)
{
    const char *const str = NULL;
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void empty_resetvector_string(void *ctx)
{
    const char *const str = "";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void resetvector(void *ctx)
{
    const char *const str = "!ab12";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_RESET, (int)vector.type);
    ct_assertequal(0xab12, vector.resetvector);
}

static void resetvector_with_space(void *ctx)
{
    const char *const str = "!   ab12";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_RESET, (int)vector.type);
    ct_assertequal(0xab12, vector.resetvector);
}

static void resetvector_with_leading_space(void *ctx)
{
    const char *const str = "   !ab12";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_RESET, (int)vector.type);
    ct_assertequal(0xab12, vector.resetvector);
}

static void resetvector_with_trailing_space(void *ctx)
{
    const char *const str = "!ab12   ";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_RESET, (int)vector.type);
    ct_assertequal(0xab12, vector.resetvector);
}

static void resetvector_caps(void *ctx)
{
    const char *const str = "!AB12";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_RESET, (int)vector.type);
    ct_assertequal(0xab12, vector.resetvector);
}

static void resetvector_with_prefix(void *ctx)
{
    const char *const str = "!0xab12";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_RESET, (int)vector.type);
    ct_assertequal(0xab12, vector.resetvector);
}

static void resetvector_short(void *ctx)
{
    const char *const str = "!1f";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_RESET, (int)vector.type);
    ct_assertequal(0x001f, vector.resetvector);
}

static void resetvector_too_large(void *ctx)
{
    const char *const str = "!12345";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(HEXPR_ERR_VALUE, result);
}

static void resetvector_negative_overflow(void *ctx)
{
    const char *const str = "!-asdf";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(HEXPR_ERR_VALUE, result);
}

static void resetvector_malformed(void *ctx)
{
    const char *const str = "!hjkl";
    struct debugexpr vector;

    const int result = haltexpr_parse_dbgexpr(str, &vector);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

static void dbgexpr_parses_halt_condition(void *ctx)
{
    const char *const str = "@ab12";
    struct debugexpr expr;

    const int result = haltexpr_parse_dbgexpr(str, &expr);

    ct_assertequal(0, result);
    ct_assertequal(DBG_EXPR_HALT, (int)expr.type);
    ct_assertequal(HLT_ADDR, (int)expr.hexpr.cond);
    ct_assertequal(0xab12u, expr.hexpr.address);
}

static void dbgexpr_malformed(void *ctx)
{
    const char *const str = "badexpr";
    struct debugexpr expr;

    const int result = haltexpr_parse_dbgexpr(str, &expr);

    ct_assertequal(HEXPR_ERR_SCAN, result);
}

#pragma mark - Expression Description
//
// Expression Description
//

static void print_none(void *ctx)
{
    const struct haltexpr expr = {0};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_desc(&expr, buf);

    const char *const expected = "None";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_addr(void *ctx)
{
    const struct haltexpr expr = {.cond = HLT_ADDR, .address = 0x1234};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_desc(&expr, buf);

    const char *const expected = "PC @ $1234";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_runtime(void *ctx)
{
    const struct haltexpr expr = {.cond = HLT_TIME, .runtime = 4.36532245f};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_desc(&expr, buf);

    const char *const expected = "4.3653226 sec";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_long_runtime(void *ctx)
{
    const struct haltexpr expr = {
        .cond = HLT_TIME, .runtime = 12312423242344.234f,
    };
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_desc(&expr, buf);

    const char *const expected = "1.2312423e+13 sec";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_cycles(void *ctx)
{
    const struct haltexpr expr = {.cond = HLT_CYCLES, .cycles = 982423};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_desc(&expr, buf);

    const char *const expected = "982423 cyc";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void print_jam(void *ctx)
{
    const struct haltexpr expr = {.cond = HLT_JAM};
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_desc(&expr, buf);

    const char *const expected = "CPU JAMMED";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

#pragma mark - Debug Expression Serialization
//
// Debug Expression Serialization
//

static void format_reset(void *ctx)
{
    const struct debugexpr expr = {
        .type = DBG_EXPR_RESET, .resetvector = 0x1234,
    };
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt_dbgexpr(&expr, buf);

    const char *const expected = "!1234";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void format_addr(void *ctx)
{
    const struct debugexpr expr = {
        .type = DBG_EXPR_HALT,
        .hexpr = {.cond = HLT_ADDR, .address = 0x1234},
    };
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt_dbgexpr(&expr, buf);

    const char *const expected = "@1234";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void format_runtime(void *ctx)
{
    const struct debugexpr expr = {
        .type = DBG_EXPR_HALT,
        .hexpr = {.cond = HLT_TIME, .runtime = 4.36532245f},
    };
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt_dbgexpr(&expr, buf);

    const char *const expected = "4.3653226s";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void format_long_runtime(void *ctx)
{
    const struct debugexpr expr = {
        .type = DBG_EXPR_HALT,
        .hexpr = {.cond = HLT_TIME, .runtime = 12312423242344.234f},
    };
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt_dbgexpr(&expr, buf);

    const char *const expected = "1.2312423e+13s";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void format_cycles(void *ctx)
{
    const struct debugexpr expr = {
        .type = DBG_EXPR_HALT,
        .hexpr = {.cond = HLT_CYCLES, .cycles = 982423},
    };
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt_dbgexpr(&expr, buf);

    const char *const expected = "982423c";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

static void format_jam(void *ctx)
{
    const struct debugexpr expr = {
        .type = DBG_EXPR_HALT, .hexpr = {.cond = HLT_JAM},
    };
    char buf[HEXPR_FMT_SIZE];

    const int result = haltexpr_fmt_dbgexpr(&expr, buf);

    const char *const expected = "JAM";
    ct_assertequal((int)strlen(expected), result);
    ct_assertequalstrn(expected, buf, sizeof expected);
}

#pragma mark - Test List
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
        ct_maketest(address_condition_too_large),
        ct_maketest(address_condition_negative_overflow),
        ct_maketest(address_condition_malformed),

        ct_maketest(runtime_condition),
        ct_maketest(runtime_condition_zero),
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

        ct_maketest(null_resetvector_string),
        ct_maketest(empty_resetvector_string),
        ct_maketest(resetvector),
        ct_maketest(resetvector_with_space),
        ct_maketest(resetvector_with_leading_space),
        ct_maketest(resetvector_with_trailing_space),
        ct_maketest(resetvector_caps),
        ct_maketest(resetvector_with_prefix),
        ct_maketest(resetvector_short),
        ct_maketest(resetvector_too_large),
        ct_maketest(resetvector_negative_overflow),
        ct_maketest(resetvector_malformed),
        ct_maketest(dbgexpr_parses_halt_condition),
        ct_maketest(dbgexpr_malformed),

        ct_maketest(print_none),
        ct_maketest(print_addr),
        ct_maketest(print_runtime),
        ct_maketest(print_long_runtime),
        ct_maketest(print_cycles),
        ct_maketest(print_jam),

        ct_maketest(format_reset),
        ct_maketest(format_addr),
        ct_maketest(format_runtime),
        ct_maketest(format_long_runtime),
        ct_maketest(format_cycles),
        ct_maketest(format_jam),
    };

    return ct_makesuite(tests);
}
