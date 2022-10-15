//
//  cli.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "cli.h"

#include "argparse.h"
#include "cart.h"
#include "control.h"
#include "debug.h"
#include "dis.h"
#include "haltexpr.h"
#include "nes.h"
#include "ui.h"
#include "snapshot.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// NOTE: forward-declare CLI's interactive mode
int ui_curses_init(ui_loop **loop);

static cart *load_cart(const char *filename)
{
    cart *c = NULL;
    errno = 0;
    FILE *const cartfile = fopen(filename, "rb");
    if (!cartfile) {
        perror("Cannot open cart file");
        return c;
    }

    const int err = cart_create(&c, cartfile);
    if (err < 0) {
        fprintf(stderr, "Cart load failure (%d): %s\n", err, cart_errstr(err));
    }
    fclose(cartfile);

    return c;
}

static int print_cart_info(const struct control *appstate, cart *c)
{
    if (appstate->verbose) {
        puts("---=== Cart Info ===---");
    }
    cart_write_info(c, ctrl_cartfilename(appstate->cartfile), stdout,
                    appstate->verbose);
    return EXIT_SUCCESS;
}

static int disassemble_cart_prg(const struct control *appstate, cart *c)
{
    const int err = dis_cart_prg(c, appstate, stdout);
    if (err < 0) {
        fprintf(stderr, "PRG decode error (%d): %s\n", err, dis_errstr(err));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int decode_cart_chr(const struct control *appstate, cart *c)
{
    errno = 0;
    const int err = dis_cart_chr(c, appstate, stdout);
    if (err < 0) {
        fprintf(stderr, "CHR decode error (%d): %s\n", err, dis_errstr(err));
        if (err == DIS_ERR_ERNO) {
            perror("CHR decode file error");
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static debugctx *create_debugger(const struct control *appstate)
{
    debugctx *const dbg = debug_new(appstate);
    for (const struct haltarg *arg = appstate->haltlist;
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
            if (appstate->verbose) {
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

static int init_ui(const struct control *appstate, ui_loop **loop)
{
    return appstate->batch ? ui_batch_init(loop) : ui_curses_init(loop);
}

static int run_emu(struct control *appstate, cart *c)
{
    if (appstate->batch && appstate->tron && !appstate->haltlist) {
        fputs("*** WARNING ***\nYou have turned on trace-logging"
              " with batch mode but specified no halt conditions;\n"
              "this can result in a very large trace file very quickly!\n"
              "Continue? [yN] ", stderr);
        const int input = getchar();
        if (input != 'y' && input != 'Y') return EXIT_FAILURE;
    }

    debugctx *dbg = create_debugger(appstate);
    if (!dbg) return EXIT_FAILURE;

    int result = EXIT_SUCCESS;
    nes *console = nes_new(c, dbg, appstate);
    if (!console) {
        result = EXIT_FAILURE;
        goto exit_debug;
    }
    nes_powerup(console);
    // NOTE: when in batch mode set NES to run immediately
    if (appstate->batch) {
        nes_mode(console, NEXC_RUN);
        nes_ready(console);
    }
    // NOTE: initialize snapshot from console
    struct console_state snapshot;
    nes_snapshot(console, &snapshot);

    ui_loop *ui_loop;
    errno = 0;
    const int err = init_ui(appstate, &ui_loop);
    if (err < 0) {
        fprintf(stderr, "UI init failure (%d): %s\n", err, ui_errstr(err));
        if (err == UI_ERR_ERNO) {
            perror("UI System Error");
        }
        result = EXIT_FAILURE;
        goto exit_console;
    }

    ui_loop(appstate, console, &snapshot);
exit_console:
    snapshot_clear(&snapshot);
    nes_free(console);
    console = NULL;
exit_debug:
    debug_free(dbg);
    dbg = NULL;
    return result;
}

static int run_cart(struct control *appstate, cart *c)
{
    if (appstate->info) return print_cart_info(appstate, c);
    if (appstate->disassemble) return disassemble_cart_prg(appstate, c);
    if (appstate->chrdecode) return decode_cart_chr(appstate, c);
    return run_emu(appstate, c);
}

static int run_with_args(struct control *appstate)
{
    if (appstate->help) {
        argparse_usage(appstate->me);
        return EXIT_SUCCESS;
    }

    if (appstate->version) {
        argparse_version();
        return EXIT_SUCCESS;
    }

    if (!appstate->cartfile) {
        fputs("No input file specified\n", stderr);
        argparse_usage(appstate->me);
        return EXIT_FAILURE;
    }

    cart *cart = load_cart(appstate->cartfile);
    if (!cart) {
        return EXIT_FAILURE;
    }

    const int result = run_cart(appstate, cart);
    cart_free(cart);
    cart = NULL;
    return result;
}

//
// Public Interface
//

int cli_run(int argc, char *argv[argc+1])
{
    struct control appstate;
    if (!argparse_parse(&appstate, argc, argv)) return EXIT_FAILURE;

    const int result = run_with_args(&appstate);
    argparse_cleanup(&appstate);
    return result;
}
