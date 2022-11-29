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
#include "haltexpr.h"
#include "nes.h"
#include "snapshot.h"
#include "ui.h"
#include "version.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef int ui_loop(const struct cliargs *, nes *, struct console_state *);
ui_loop ui_batch_loop;
ui_loop ui_curses_loop;
const char *ui_curses_version(void);

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
    errno = 0;
    const int err = cart_create(&c, filename);
    if (err < 0) {
        fprintf(stderr, "Cart load failure (%d): %s\n", err, cart_errstr(err));
        if (err == CART_ERR_ERNO) {
            perror("Cannot open cart file");
        }
    }
    return c;
}

static int print_cart_info(const struct cliargs *args, cart *c)
{
    if (args->verbose) {
        puts("---=== Cart Info ===---");
    }
    cart_write_info(c, args->verbose, stdout);
    return EXIT_SUCCESS;
}

static int disassemble_cart_prg(const struct cliargs *args, cart *c)
{
    const int err = dis_cart_prg(c, args->verbose, false, stdout);
    if (err < 0) {
        fprintf(stderr, "PRG decode error (%d): %s\n", err, dis_errstr(err));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int decode_cart_chr(const struct cliargs *args, cart *c)
{
    errno = 0;
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

static debugctx *create_debugger(const struct cliargs *args)
{
    debugctx *const dbg = debug_new();
    debug_set_reset(dbg, args->resetvector);
    for (const struct haltarg *arg = args->haltlist;
         arg;
         arg = arg->next) {
        struct haltexpr expr;
        int err = haltexpr_parse(arg->expr, &expr);
        if (err < 0) {
            fprintf(stderr, "Halt expression parse failure (%d): %s"
                    " > \"%s\"\n", err, haltexpr_errstr(err), arg->expr);
            debug_free(dbg);
            return NULL;
        } else {
            debug_addbreakpoint(dbg, expr);
            if (args->verbose) {
                char buf[HEXPR_FMT_SIZE];
                err = haltexpr_fmt(&expr, buf);
                if (err < 0) {
                    fprintf(stderr, "Halt expr display error (%d): %s\n", err,
                            haltexpr_errstr(err));
                } else {
                    printf("Halt Condition: %s\n", buf);
                }
            }
        }
    }
    return dbg;
}

static ui_loop *setup_ui(const struct cliargs *args, nes *console,
                         struct console_state *snapshot)
{
    ui_loop *loop = ui_curses_loop;
    if (args->batch) {
        // NOTE: when in batch mode set NES to run immediately
        nes_mode(console, CSGM_RUN);
        nes_ready(console);
        loop = ui_batch_loop;
    }
    // NOTE: initialize snapshot from console
    nes_snapshot(console, snapshot);
    return loop;
}

static void dump_ram(const struct cliargs *args, nes *console)
{
    static const char *const restrict ramfile = "system.ram";

    if (args->tron || args->batch) {
        errno = 0;
        const int err = nes_dumpram(console, ramfile);
        if (err < 0) {
            fprintf(stderr, "Ram dump failure: %s - %s", ramfile,
                    nes_errstr(err));
            if (err == NES_ERR_ERNO) {
                perror("System error");
            }
        }
    }
}

static int run_emu(const struct cliargs *args, cart *c)
{
    if (args->batch && args->tron && !args->haltlist) {
        fputs("*** WARNING ***\nYou have turned on trace-logging"
              " with batch mode but specified no halt conditions;\n"
              "this can result in a very large trace file very quickly!\n"
              "Continue? [yN] ", stderr);
        const int input = getchar();
        if (input != 'y' && input != 'Y') return EXIT_FAILURE;
    }

    debugctx *dbg = create_debugger(args);
    if (!dbg) return EXIT_FAILURE;

    int result = EXIT_SUCCESS;
    nes *console = nes_new(dbg, args->bcdsupport, args->tron);
    if (!console) {
        result = EXIT_FAILURE;
        goto exit_debug;
    }
    nes_powerup(console, c, args->zeroram);

    struct console_state snapshot;
    ui_loop *const run_loop = setup_ui(args, console, &snapshot);
    errno = 0;
    const int err = run_loop(args, console, &snapshot);
    if (err < 0) {
        fprintf(stderr, "UI run failure (%d): %s\n", err, ui_errstr(err));
        if (err == UI_ERR_ERNO) {
            perror("UI System Error");
        }
        result = EXIT_FAILURE;
    }
    dump_ram(args, console);
    snapshot_clear(&snapshot);
    nes_free(console);
    console = NULL;
exit_debug:
    debug_free(dbg);
    dbg = NULL;
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

    if (!args->cartfile) {
        fputs("No input file specified\n", stderr);
        argparse_usage(args->me);
        return EXIT_FAILURE;
    }

    cart *cart = load_cart(args->cartfile);
    if (!cart) {
        return EXIT_FAILURE;
    }

    const int result = run_cart(args, cart);
    cart_free(cart);
    cart = NULL;
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
