//
//  argparse.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 1/5/22.
//

#include "argparse.h"
#include "ciny.h"
#include "control.h"

#include <stdbool.h>
#include <stddef.h>

static void init_control_zero_args(void *ctx)
{
    struct control appstate;
    char *argv[] = {NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequalstr("aldo", appstate.me);
    ct_assertequal(1, appstate.chrscale);
    ct_assertequal(4, appstate.clock.cycles_per_sec);
    ct_assertequal(-1, appstate.haltaddr);
    ct_assertequal(-1, appstate.resetvector);
    ct_asserttrue(appstate.help);
    ct_asserttrue(appstate.running);

    ct_assertequal(0u, appstate.clock.total_cycles);
    ct_assertequal(0, appstate.clock.budget);
    ct_assertnull(appstate.cartfile);
    ct_assertnull(appstate.chrdecode_prefix);
    ct_assertequal(0, appstate.ramsheet);
    ct_assertfalse(appstate.batch);
    ct_assertfalse(appstate.chrdecode);
    ct_assertfalse(appstate.disassemble);
    ct_assertfalse(appstate.info);
    ct_assertfalse(appstate.tron);
    ct_assertfalse(appstate.verbose);
    ct_assertfalse(appstate.version);
}

static void cli_zero_args(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequalstr("testaldo", appstate.me);
    ct_asserttrue(appstate.help);
}

static void single_arg(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "test.rom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequalstr("testaldo", appstate.me);
    ct_assertequalstr("test.rom", appstate.cartfile);
    ct_assertfalse(appstate.help);
}

static void flag_short(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-b", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.batch);
    ct_assertfalse(appstate.help);
}

static void flag_long(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--batch", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.batch);
    ct_assertfalse(appstate.help);
}

static void multiple_flags(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-d", "-v", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.disassemble);
    ct_asserttrue(appstate.verbose);
}

static void mix_long_and_short(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--disassemble", "-v", "--trace", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.disassemble);
    ct_asserttrue(appstate.tron);
    ct_asserttrue(appstate.verbose);
}

static void combined_flags(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-dvt", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.disassemble);
    ct_asserttrue(appstate.tron);
    ct_asserttrue(appstate.verbose);
}

static void short_chr_scale(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-s" "5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, appstate.chrscale);
}

static void short_chr_scale_no_space(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-s5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, appstate.chrscale);
}

static void short_chr_scale_with_equals(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-s=5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, appstate.chrscale);
}

static void short_chr_scale_out_of_range(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-s20", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, appstate.chrscale);
}

static void short_chr_scale_malformed(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-sabc", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, appstate.chrscale);
}

static void long_chr_scale(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--chr-scale=5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, appstate.chrscale);
}

static void long_chr_scale_with_space(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--chr-scale", "5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, appstate.chrscale);
}

static void long_chr_scale_out_of_range(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--chr-scale=20", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, appstate.chrscale);
}

static void long_chr_scale_malformed(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--chr-scale=abc", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, appstate.chrscale);
}

static void chr_decode_short(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-c", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.chrdecode);
}

static void chr_decode_short_does_not_support_prefix(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-c=myrom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.chrdecode);
    ct_assertnull(appstate.chrdecode_prefix);
}

static void chr_decode_long_no_prefix(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--chr-decode", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.chrdecode);
    ct_assertnull(appstate.chrdecode_prefix);
}

static void chr_decode_long_with_prefix(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--chr-decode=myrom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.chrdecode);
    ct_assertequalstr("myrom", appstate.chrdecode_prefix);
}

static void both_flags_and_values(void *ctx)
{
    struct control appstate;
    char *argv[] = {
        "testaldo", "-dv", "-s5", "--chr-decode=myrom", "test.rom", NULL,
    };
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(appstate.disassemble);
    ct_asserttrue(appstate.verbose);
    ct_assertequal(5, appstate.chrscale);
    ct_asserttrue(appstate.chrdecode);
    ct_assertequalstr("myrom", appstate.chrdecode_prefix);
    ct_assertequalstr("test.rom", appstate.cartfile);
}

static void flags_and_values_do_not_combine(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-ds", "5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    // NOTE: 's' param is skipped so '5' is misparsed as the cart file
    ct_asserttrue(appstate.disassemble);
    ct_assertequal(1, appstate.chrscale);
    ct_assertequalstr("5", appstate.cartfile);
}

static void reset_override_short(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-r", "abcd", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(0xabcd, appstate.resetvector);
}

static void reset_override_short_out_of_range(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-r", "12345", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, appstate.resetvector);
}

static void reset_override_short_malformed(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "-r", "notahexnum", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, appstate.resetvector);
}

static void reset_override_long(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--reset-vector=abcd", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(0xabcd, appstate.resetvector);
}

static void reset_override_long_out_of_range(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--reset-vector=98765", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, appstate.resetvector);
}

static void reset_override_long_malformed(void *ctx)
{
    struct control appstate;
    char *argv[] = {"testaldo", "--reset-vector=nothex", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(&appstate, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, appstate.resetvector);
}

//
// Test List
//

struct ct_testsuite argparse_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(init_control_zero_args),
        ct_maketest(cli_zero_args),
        ct_maketest(single_arg),

        ct_maketest(flag_short),
        ct_maketest(flag_long),
        ct_maketest(multiple_flags),
        ct_maketest(mix_long_and_short),
        ct_maketest(combined_flags),

        ct_maketest(short_chr_scale),
        ct_maketest(short_chr_scale_no_space),
        ct_maketest(short_chr_scale_with_equals),
        ct_maketest(short_chr_scale_out_of_range),
        ct_maketest(short_chr_scale_malformed),
        ct_maketest(long_chr_scale),
        ct_maketest(long_chr_scale_with_space),
        ct_maketest(long_chr_scale_out_of_range),
        ct_maketest(long_chr_scale_malformed),

        ct_maketest(chr_decode_short),
        ct_maketest(chr_decode_short_does_not_support_prefix),
        ct_maketest(chr_decode_long_no_prefix),
        ct_maketest(chr_decode_long_with_prefix),

        ct_maketest(both_flags_and_values),
        ct_maketest(flags_and_values_do_not_combine),

        ct_maketest(reset_override_short),
        ct_maketest(reset_override_short_out_of_range),
        ct_maketest(reset_override_short_malformed),
        ct_maketest(reset_override_long),
        ct_maketest(reset_override_long_out_of_range),
        ct_maketest(reset_override_long_malformed),
    };

    return ct_makesuite(tests);
}
