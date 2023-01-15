//
//  argparse.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 1/5/22.
//

#include "argparse.h"
#include "ciny.h"
#include "cliargs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static void setup(void **ctx)
{
    *ctx = malloc(sizeof(struct cliargs));
}

static void teardown(void **ctx)
{
    struct cliargs *const args = *ctx;
    argparse_cleanup(args);
    free(args);
}

static void init_control_zero_args(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequalstr("aldo", args->me);
    ct_assertequal(1, args->chrscale);
    ct_assertequal(-1, args->resetvector);
    ct_asserttrue(args->help);

    ct_assertnull(args->filepath);
    ct_assertnull(argparse_filename(args->filepath));
    ct_assertnull(args->chrdecode_prefix);
    ct_assertnull(args->haltlist);
    ct_assertnull(args->dbgfilepath);
    ct_assertfalse(args->batch);
    ct_assertfalse(args->chrdecode);
    ct_assertfalse(args->disassemble);
    ct_assertfalse(args->info);
    ct_assertfalse(args->tron);
    ct_assertfalse(args->verbose);
    ct_assertfalse(args->version);
}

static void cli_zero_args(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequalstr("testaldo", args->me);
    ct_asserttrue(args->help);
}

static void single_arg(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "test.rom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequalstr("testaldo", args->me);
    ct_assertequalstr("test.rom", args->filepath);
    ct_assertequalstr("test.rom", argparse_filename(args->filepath));
    ct_assertfalse(args->help);
}

static void full_filepath(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "/foo/bar/test.rom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequalstr("testaldo", args->me);
    ct_assertequalstr("/foo/bar/test.rom", args->filepath);
    ct_assertequalstr("test.rom", argparse_filename(args->filepath));
    ct_assertfalse(args->help);
}

static void flag_short(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-b", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->batch);
    ct_assertfalse(args->help);
}

static void flag_long(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--batch", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->batch);
    ct_assertfalse(args->help);
}

static void multiple_flags(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-d", "-v", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->disassemble);
    ct_asserttrue(args->verbose);
}

static void mix_long_and_short(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--disassemble", "-v", "--trace", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->disassemble);
    ct_asserttrue(args->tron);
    ct_asserttrue(args->verbose);
}

static void combined_flags(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-dvt", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->disassemble);
    ct_asserttrue(args->tron);
    ct_asserttrue(args->verbose);
}

static void chr_scale_short(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-s", "5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, args->chrscale);
}

static void chr_scale_short_no_space(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-s5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, args->chrscale);
}

static void chr_scale_short_does_not_support_equals(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-s=5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_scale_short_out_of_range(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-s", "20", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_scale_short_malformed(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-s", "abc", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_scale_short_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-s", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_scale_long(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-scale", "5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, args->chrscale);
}

static void chr_scale_long_with_equals(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-scale=5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(5, args->chrscale);
}

static void chr_scale_long_out_of_range(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-scale", "20", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_scale_long_malformed(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-scale", "abc", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_scale_long_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-scale", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_scale_long_does_not_overparse(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-scalefoobar", "5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(1, args->chrscale);
}

static void chr_decode_short(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-c", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->chrdecode);
}

static void chr_decode_short_does_not_support_prefix(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-c=myrom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->chrdecode);
    ct_assertnull(args->chrdecode_prefix);
}

static void chr_decode_long_no_prefix(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-decode", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->chrdecode);
    ct_assertnull(args->chrdecode_prefix);
}

static void chr_decode_long_with_prefix(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-decode=myrom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->chrdecode);
    ct_assertequalstr("myrom", args->chrdecode_prefix);
}

static void chr_decode_long_does_not_overparse(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--chr-decodefoobar=myrom", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertfalse(args->chrdecode);
    ct_assertnull(args->chrdecode_prefix);
}

static void both_flags_and_values(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {
        "testaldo", "-dv", "-s", "5", "--chr-decode=myrom", "test.rom", NULL,
    };
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->disassemble);
    ct_asserttrue(args->verbose);
    ct_assertequal(5, args->chrscale);
    ct_asserttrue(args->chrdecode);
    ct_assertequalstr("myrom", args->chrdecode_prefix);
    ct_assertequalstr("test.rom", args->filepath);
    ct_assertequalstr("test.rom", argparse_filename(args->filepath));
}

static void flags_and_values_do_not_combine(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-ds", "5", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    // NOTE: 's' param is skipped so '5' is misparsed as the cart file
    ct_asserttrue(args->disassemble);
    ct_assertequal(1, args->chrscale);
    ct_assertequalstr("5", args->filepath);
    ct_assertequalstr("5", argparse_filename(args->filepath));
}

static void reset_override_short(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-r", "abcd", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(0xabcd, args->resetvector);
}

static void reset_override_short_no_space(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-rabcd", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(0xabcd, args->resetvector);
}

static void reset_override_short_out_of_range(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-r", "12345", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, args->resetvector);
}

static void reset_override_short_malformed(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-r", "notahexnum", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, args->resetvector);
}

static void reset_override_short_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-r", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, args->resetvector);
}

static void reset_override_long(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--reset-vector", "abcd", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(0xabcd, args->resetvector);
}

static void reset_override_long_with_equals(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--reset-vector=abcd", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(0xabcd, args->resetvector);
}

static void reset_override_long_out_of_range(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--reset-vector", "98765", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, args->resetvector);
}

static void reset_override_long_malformed(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--reset-vector", "nothex", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, args->resetvector);
}

static void reset_override_long_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--reset-vector", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertequal(-1, args->resetvector);
}

static void reset_override_long_does_not_overparse(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--reset-vectorfoobar", "98765", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertequal(-1, args->resetvector);
}

static void halt_short(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-H", "foo", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->haltlist);
    ct_assertequalstr("foo", args->haltlist->expr);
    ct_assertnull(args->haltlist->next);
}

static void halt_short_no_space(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-Hfoo", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->haltlist);
    ct_assertequalstr("foo", args->haltlist->expr);
    ct_assertnull(args->haltlist->next);
}

static void halt_short_multiple(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-H", "foo", "-H", "bar", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->haltlist);
    ct_assertequalstr("foo", args->haltlist->expr);
    ct_assertnotnull(args->haltlist->next);
    ct_assertequalstr("bar", args->haltlist->next->expr);
    ct_assertnull(args->haltlist->next->next);
}

static void halt_short_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-H", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertnull(args->haltlist);
}

static void halt_long(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--halt", "foo", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->haltlist);
    ct_assertequalstr("foo", args->haltlist->expr);
    ct_assertnull(args->haltlist->next);
}

static void halt_long_with_equals(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--halt=foo", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->haltlist);
    ct_assertequalstr("foo", args->haltlist->expr);
    ct_assertnull(args->haltlist->next);
}

static void halt_long_multiple(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--halt", "foo", "--halt", "bar", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->haltlist);
    ct_assertequalstr("foo", args->haltlist->expr);
    ct_assertnotnull(args->haltlist->next);
    ct_assertequalstr("bar", args->haltlist->next->expr);
    ct_assertnull(args->haltlist->next->next);
}

static void halt_long_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--halt", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertnull(args->haltlist);
}

static void halt_long_does_not_overparse(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--haltfoobar", "baz", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnull(args->haltlist);
}

static void debug_file_short(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-g", "my/debug/file", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->dbgfilepath);
    ct_assertequalstr("my/debug/file", args->dbgfilepath);
}

static void debug_file_short_no_space(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-gmy/debug/file", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->dbgfilepath);
    ct_assertequalstr("my/debug/file", args->dbgfilepath);
}

static void debug_file_short_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-g", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertnull(args->dbgfilepath);
}

static void debug_file_long(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--dbg-file", "my/debug/file", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->dbgfilepath);
    ct_assertequalstr("my/debug/file", args->dbgfilepath);
}

static void debug_file_long_with_equals(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--dbg-file=my/debug/file", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->dbgfilepath);
    ct_assertequalstr("my/debug/file", args->dbgfilepath);
}

static void debug_file_long_missing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--dbg-file", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_assertfalse(result);

    ct_assertnull(args->dbgfilepath);
}

static void debug_file_long_does_not_overparse(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "--dbg-filemy/debug/file", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnull(args->dbgfilepath);
}

static void option_does_not_trigger_flag(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {"testaldo", "-Hdv", NULL};
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_assertnotnull(args->haltlist);
    ct_assertequalstr("dv", args->haltlist->expr);
    ct_assertnull(args->haltlist->next);
    ct_assertfalse(args->disassemble);
    ct_assertfalse(args->verbose);
}

static void double_dash_ends_option_parsing(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {
        "testaldo",
        "-dv", "--", "-b", "-s", "5", "--chr-decode=myrom", "test.rom", NULL,
    };
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->disassemble);
    ct_asserttrue(args->verbose);
    ct_assertfalse(args->batch);
    ct_assertequal(1, args->chrscale);
    ct_assertfalse(args->chrdecode);
    ct_assertnull(args->chrdecode_prefix);
    ct_assertequalstr("test.rom", args->filepath);
    ct_assertequalstr("test.rom", argparse_filename(args->filepath));
}

static void double_dash_ends_option_parsing_unordered(void *ctx)
{
    struct cliargs *const args = ctx;
    char *argv[] = {
        "testaldo",
        "-dv", "--", "-b", "test.rom", "-s5", "--chr-decode=myrom", NULL,
    };
    const int argc = (sizeof argv / sizeof argv[0]) - 1;

    const bool result = argparse_parse(args, argc, argv);

    ct_asserttrue(result);

    ct_asserttrue(args->disassemble);
    ct_asserttrue(args->verbose);
    ct_assertfalse(args->batch);
    ct_assertequal(1, args->chrscale);
    ct_assertfalse(args->chrdecode);
    ct_assertnull(args->chrdecode_prefix);
    ct_assertequalstr("test.rom", args->filepath);
    ct_assertequalstr("test.rom", argparse_filename(args->filepath));
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
        ct_maketest(full_filepath),

        ct_maketest(flag_short),
        ct_maketest(flag_long),
        ct_maketest(multiple_flags),
        ct_maketest(mix_long_and_short),
        ct_maketest(combined_flags),

        ct_maketest(chr_scale_short),
        ct_maketest(chr_scale_short_no_space),
        ct_maketest(chr_scale_short_does_not_support_equals),
        ct_maketest(chr_scale_short_out_of_range),
        ct_maketest(chr_scale_short_malformed),
        ct_maketest(chr_scale_short_missing),
        ct_maketest(chr_scale_long),
        ct_maketest(chr_scale_long_with_equals),
        ct_maketest(chr_scale_long_out_of_range),
        ct_maketest(chr_scale_long_malformed),
        ct_maketest(chr_scale_long_missing),
        ct_maketest(chr_scale_long_does_not_overparse),

        ct_maketest(chr_decode_short),
        ct_maketest(chr_decode_short_does_not_support_prefix),
        ct_maketest(chr_decode_long_no_prefix),
        ct_maketest(chr_decode_long_with_prefix),
        ct_maketest(chr_decode_long_does_not_overparse),

        ct_maketest(both_flags_and_values),
        ct_maketest(flags_and_values_do_not_combine),

        ct_maketest(reset_override_short),
        ct_maketest(reset_override_short_no_space),
        ct_maketest(reset_override_short_out_of_range),
        ct_maketest(reset_override_short_malformed),
        ct_maketest(reset_override_short_missing),
        ct_maketest(reset_override_long),
        ct_maketest(reset_override_long_with_equals),
        ct_maketest(reset_override_long_out_of_range),
        ct_maketest(reset_override_long_malformed),
        ct_maketest(reset_override_long_missing),
        ct_maketest(reset_override_long_does_not_overparse),

        ct_maketest(halt_short),
        ct_maketest(halt_short_no_space),
        ct_maketest(halt_short_multiple),
        ct_maketest(halt_short_missing),
        ct_maketest(halt_long),
        ct_maketest(halt_long_with_equals),
        ct_maketest(halt_long_multiple),
        ct_maketest(halt_long_missing),
        ct_maketest(halt_long_does_not_overparse),

        ct_maketest(debug_file_short),
        ct_maketest(debug_file_short_no_space),
        ct_maketest(debug_file_short_missing),
        ct_maketest(debug_file_long),
        ct_maketest(debug_file_long_with_equals),
        ct_maketest(debug_file_long_missing),
        ct_maketest(debug_file_long_does_not_overparse),

        ct_maketest(option_does_not_trigger_flag),
        ct_maketest(double_dash_ends_option_parsing),
        ct_maketest(double_dash_ends_option_parsing_unordered),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
