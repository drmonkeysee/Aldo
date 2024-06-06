//
//  cli.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "cli.h"

#include "argparse.h"
#include "cart.h"
#include "cliargs.h"
#include "ctrlsignal.h"
#include "debug.h"
#include "dis.h"
#include "emu.h"
#include "haltexpr.h"
#include "nes.h"
#include "snapshot.h"
#include "ui.h"
#include "version.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef int ui_loop(struct emulator *);
ui_loop ui_batch_loop;
ui_loop ui_curses_loop;
const char *ui_curses_version(void);

static const char *const restrict ResetOverrideFmt =
    "RESET Override: " HEXPR_RES_IND "%04X\n";

static void print_version(void)
{
    printf("Aldo %s", AldoVersion);
#ifdef __VERSION__
    fputs(" (" __VERSION__ ")", stdout);
#endif
    printf("\n%s\n", ui_curses_version());
}

static cart *load_cart(const char *filename)
{
    cart *c = NULL;
    FILE *const f = fopen(filename, "rb");
    if (f) {
        const int err = cart_create(&c, f);
        if (err < 0) {
            fprintf(stderr, "Cart load failure (%d): %s\n", err,
                    cart_errstr(err));
        }
        fclose(f);
    } else {
        fprintf(stderr, "%s: ", filename);
        perror("Cannot open cart file");
    }
    return c;
}

static int print_cart_info(const struct cliargs *args, cart *c)
{
    if (args->verbose) {
        puts("---=== Cart Info ===---");
    }
    const char *const name = args->verbose
                                ? args->filepath
                                : argparse_filename(args->filepath);
    cart_write_info(c, name, args->verbose, stdout);
    return EXIT_SUCCESS;
}

static int disassemble_cart_prg(const struct cliargs *args, cart *c)
{
    const int err = dis_cart_prg(c, argparse_filename(args->filepath),
                                 args->verbose, false, stdout);
    if (err < 0) {
        fprintf(stderr, "PRG decode error (%d): %s\n", err, dis_errstr(err));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int decode_cart_chr(const struct cliargs *args, cart *c)
{
    const int err = dis_cart_chr(c, args->chrscale, args->chrdecode_prefix,
                                 stdout);
    if (err < 0) {
        fprintf(stderr, "CHR decode error (%d): %s\n", err, dis_errstr(err));
        if (err == DIS_ERR_ERNO) {
            perror("CHR decode file error");
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static bool parse_dbg_expression(debugger *dbg, const char *restrict exprstr,
                                 bool verbose)
{
    struct debugexpr expr;
    int err = haltexpr_parse_dbgexpr(exprstr, &expr);
    if (err < 0) {
        fprintf(stderr,
                "Debug expression parse failure (%d): %s > \"%s\"\n", err,
                haltexpr_errstr(err), exprstr);
        return false;
    } else {
        if (expr.type == DBG_EXPR_HALT) {
            debug_bp_add(dbg, expr.hexpr);
            if (verbose) {
                char buf[HEXPR_FMT_SIZE];
                err = haltexpr_desc(&expr.hexpr, buf);
                if (err < 0) {
                    fprintf(stderr, "Halt expr display error (%d): %s\n", err,
                            haltexpr_errstr(err));
                } else {
                    printf("Halt Condition: %s\n", buf);
                }
            }
        } else {
            debug_set_vector_override(dbg, expr.resetvector);
            if (verbose) {
                printf(ResetOverrideFmt, expr.resetvector);
            }
        }
    }
    return true;
}

static bool parse_debug_file(debugger *dbg, FILE *f,
                             const struct cliargs *args)
{
    char buf[HEXPR_FMT_SIZE];
    while (fgets(buf, sizeof buf, f)) {
        if (!parse_dbg_expression(dbg, buf, args->verbose)) {
            return false;
        }
    }
    if (ferror(f)) {
        fprintf(stderr, "%s: ", args->dbgfilepath);
        perror("Debug file read failure");
        return false;
    }
    return true;
}

static debugger *create_debugger(const struct cliargs *args)
{
    debugger *const dbg = debug_new();
    if (args->dbgfilepath) {
        FILE *const f = fopen(args->dbgfilepath, "r");
        if (f) {
            const bool success = parse_debug_file(dbg, f, args);
            fclose(f);
            if (!success) goto exit_dbg;
        } else {
            fprintf(stderr, "%s: ", args->dbgfilepath);
            perror("Cannot open debug file");
            goto exit_dbg;
        }
    } else {
        for (const struct haltarg *arg = args->haltlist;
             arg;
             arg = arg->next) {
            if (!parse_dbg_expression(dbg, arg->expr, args->verbose))
                goto exit_dbg;
        }
    }
    if (args->resetvector != NoResetVector) {
        debug_set_vector_override(dbg, args->resetvector);
        printf(ResetOverrideFmt, args->resetvector);
    }
    return dbg;
exit_dbg:
    debug_free(dbg);
    return NULL;
}

static ui_loop *setup_ui(struct emulator *emu)
{
    ui_loop *loop = ui_curses_loop;
    if (emu->args->batch) {
        // NOTE: when in batch mode set NES to run immediately
        nes_set_mode(emu->console, CSGM_RUN);
        nes_ready(emu->console, true);
        loop = ui_batch_loop;
    }
    // NOTE: initialize snapshot from console
    nes_snapshot(emu->console, &emu->snapshot);
    return loop;
}

static void dump_ram(const struct emulator *emu)
{
    static const char *const restrict ramfile = "system.ram";

    if (!emu->args->tron && !emu->args->batch) return;

    FILE *const f = fopen(ramfile, "wb");
    if (f) {
        nes_dumpram(emu->console, f);
        fclose(f);
    } else {
        fprintf(stderr, "%s: ", ramfile);
        perror("Cannot open ramdump file");
    }
}

static int run_emu(const struct cliargs *args, cart *c)
{
    static const char *const restrict tracefile = "trace.log";

    struct emulator emu = {
        .args = args,
        .cart = c,
        .debugger = create_debugger(args),
    };
    if (!emu.debugger) {
        fputs("Unable to initialize debugger!\n", stderr);
        return EXIT_FAILURE;
    }

    if (emu.args->batch && emu.args->tron
        && debug_bp_count(emu.debugger) == 0) {
        fputs("*** WARNING ***\nYou have turned on trace-logging"
              " with batch mode but specified no halt conditions;\n"
              "this can result in a very large trace file very quickly!\n"
              "Continue? [yN] ", stderr);
        const int input = getchar();
        if (input != 'y' && input != 'Y') return EXIT_FAILURE;
    }

    int result = EXIT_SUCCESS;
    FILE *tracelog = NULL;
    if (emu.args->tron) {
        if (!(tracelog = fopen(tracefile, "w"))) {
            fprintf(stderr, "%s: ", tracefile);
            perror("Cannot open trace file");
            result = EXIT_FAILURE;
            goto exit_debug;
        }
    }
    emu.console = nes_new(emu.debugger, emu.args->bcdsupport, tracelog);
    if (!emu.console) {
        fputs("Unable to initialize console!\n", stderr);
        result = EXIT_FAILURE;
        goto exit_trace;
    }
    nes_powerup(emu.console, c, emu.args->zeroram);

    ui_loop *const run_loop = setup_ui(&emu);
    const int err = run_loop(&emu);
    if (err < 0) {
        fprintf(stderr, "UI run failure (%d): %s\n", err, ui_errstr(err));
        if (err == UI_ERR_ERNO) {
            perror("UI System Error");
        }
        result = EXIT_FAILURE;
    }
    dump_ram(&emu);
    snapshot_clear(&emu.snapshot);
    nes_free(emu.console);
exit_trace:
    if (tracelog) {
        fclose(tracelog);
    }
exit_debug:
    debug_free(emu.debugger);
    return result;
}

static int run_cart(const struct cliargs *args, cart *c)
{
    if (args->info) return print_cart_info(args, c);
    if (args->disassemble) return disassemble_cart_prg(args, c);
    if (args->chrdecode) return decode_cart_chr(args, c);
    return run_emu(args, c);
}

static int run_with_args(const struct cliargs *args)
{
    if (args->help) {
        argparse_usage(args->me);
        return EXIT_SUCCESS;
    }

    if (args->version) {
        print_version();
        return EXIT_SUCCESS;
    }

    if (!args->filepath) {
        fputs("No input file specified\n", stderr);
        argparse_usage(args->me);
        return EXIT_FAILURE;
    }

    cart *const cart = load_cart(args->filepath);
    if (!cart) {
        return EXIT_FAILURE;
    }

    const int result = run_cart(args, cart);
    cart_free(cart);
    return result;
}

//
// Public Interface
//

int cli_run(int argc, char *argv[argc+1])
{
    struct cliargs args;
    if (!argparse_parse(&args, argc, argv)) return EXIT_FAILURE;

    const int result = run_with_args(&args);
    argparse_cleanup(&args);
    return result;
}
