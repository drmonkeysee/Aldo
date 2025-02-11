//
//  cli.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "cli.h"

#include "argparse.h"
#include "bytes.h"
#include "cart.h"
#include "cliargs.h"
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
    "RESET Override: " ALDO_HEXPR_RST_IND "%04X\n";

static void print_version(void)
{
    printf("Aldo %s", Aldo_Version);
#ifdef __VERSION__
    fputs(" (" __VERSION__ ")", stdout);
#endif
    printf("\n%s\n", ui_curses_version());
}

static aldo_cart *load_cart(const char *filename)
{
    aldo_cart *c = NULL;
    FILE *f = fopen(filename, "rb");
    if (f) {
        int err = aldo_cart_create(&c, f);
        if (err < 0) {
            fprintf(stderr, "Cart load failure (%d): %s\n", err,
                    aldo_cart_errstr(err));
            if (err == ALDO_CART_ERR_ERNO) {
                perror("Cart system error");
            }
        }
        fclose(f);
    } else {
        fprintf(stderr, "%s: ", filename);
        perror("Cannot open cart file");
    }
    return c;
}

static int print_cart_info(const struct cliargs *args, aldo_cart *c)
{
    if (args->verbose) {
        puts("---=== Cart Info ===---");
    }
    const char *name = args->verbose
                        ? args->filepath
                        : argparse_filename(args->filepath);
    int err = aldo_cart_write_info(c, name, args->verbose, stdout);
    if (err < 0) {
        fprintf(stderr, "Cart info print failure (%d): %s\n", err,
                aldo_cart_errstr(err));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int disassemble_cart_prg(const struct cliargs *args, aldo_cart *c)
{
    int err = aldo_dis_cart_prg(c, argparse_filename(args->filepath),
                                args->verbose, false, stdout);
    if (err < 0) {
        fprintf(stderr, "PRG decode error (%d): %s\n", err,
                aldo_dis_errstr(err));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int decode_cart_chr(const struct cliargs *args, aldo_cart *c)
{
    int err = aldo_dis_cart_chr(c, args->chrscale, args->chrdecode_prefix,
                                stdout);
    if (err < 0) {
        fprintf(stderr, "CHR decode error (%d): %s\n", err,
                aldo_dis_errstr(err));
        if (err == ALDO_DIS_ERR_ERNO) {
            perror("CHR decode system error");
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static bool parse_dbg_expression(aldo_debugger *dbg,
                                 const char *restrict exprstr, bool verbose)
{
    struct aldo_debugexpr expr;
    int err = aldo_haltexpr_parse_dbg(exprstr, &expr);
    if (err < 0) {
        fprintf(stderr,
                "Debug expression parse failure (%d): %s > \"%s\"\n", err,
                aldo_haltexpr_errstr(err), exprstr);
        return false;
    } else {
        if (expr.type == ALDO_DBG_EXPR_HALT) {
            if (!aldo_debug_bp_add(dbg, expr.hexpr)) {
                perror("Unable to add debug expression");
                return false;
            }
            if (verbose) {
                char buf[ALDO_HEXPR_FMT_SIZE];
                err = aldo_haltexpr_desc(&expr.hexpr, buf);
                if (err < 0) {
                    fprintf(stderr, "Halt expr display error (%d): %s\n", err,
                            aldo_haltexpr_errstr(err));
                } else {
                    printf("Halt Condition: %s\n", buf);
                }
            }
        } else {
            aldo_debug_set_vector_override(dbg, expr.resetvector);
            if (verbose) {
                printf(ResetOverrideFmt, expr.resetvector);
            }
        }
    }
    return true;
}

static bool parse_debug_file(aldo_debugger *dbg, FILE *f,
                             const struct cliargs *args)
{
    char buf[ALDO_HEXPR_FMT_SIZE];
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

static aldo_debugger *create_debugger(const struct cliargs *args)
{
    aldo_debugger *dbg = aldo_debug_new();
    if (!dbg) {
        perror("Unable to initialize debugger");
        return dbg;
    }
    if (args->dbgfilepath) {
        FILE *f = fopen(args->dbgfilepath, "r");
        if (f) {
            bool success = parse_debug_file(dbg, f, args);
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
    if (args->resetvector != Aldo_NoResetVector) {
        aldo_debug_set_vector_override(dbg, args->resetvector);
        printf(ResetOverrideFmt, args->resetvector);
    }
    return dbg;
exit_dbg:
    aldo_debug_free(dbg);
    return NULL;
}

static ui_loop *setup_ui(struct emulator *emu)
{
    ui_loop *loop = ui_curses_loop;
    if (emu->args->batch) {
        aldo_nes_ready(emu->console, true);
        loop = ui_batch_loop;
    }
    aldo_nes_set_snapshot(emu->console, &emu->snapshot);
    return loop;
}

static void dump_ram(const struct emulator *emu)
{
    static const char *const restrict dmpfiles[] = {
        "ram.bin",
        "vram.bin",
        "ppu.bin",
    };
    // TODO: constexpr in c23 will remove the VLAs below
    static const size_t dmpcount = aldo_arrsz(dmpfiles);

    if (!emu->args->tron && !emu->args->batch) return;

    FILE *fs[dmpcount];
    for (size_t i = 0; i < dmpcount; ++i) {
        fs[i] = fopen(dmpfiles[i], "wb");
        if (!fs[i]) {
            fprintf(stderr, "%s: ", dmpfiles[i]);
            perror("Cannot open ramdump file");
        }
    }
    bool errs[dmpcount];
    aldo_nes_dumpram(emu->console, fs, errs);
    for (size_t i = 0; i < dmpcount; ++i) {
        if (fs[i]) {
            if (errs[i]) {
                fprintf(stderr, "Write ramdump failure %s: ", dmpfiles[i]);
            }
            fclose(fs[i]);
        }
    }
}

static int run_emu(const struct cliargs *args, aldo_cart *c)
{
    static const char *const restrict tracefile = "trace.log";

    struct emulator emu = {
        .args = args,
        .cart = c,
        .debugger = create_debugger(args),
    };
    if (!emu.debugger) return EXIT_FAILURE;

    if (emu.args->batch && emu.args->tron
        && aldo_debug_bp_count(emu.debugger) == 0) {
        fputs("*** WARNING ***\nYou have turned on trace-logging"
              " with batch mode but specified no halt conditions;\n"
              "this can result in a very large trace file very quickly!\n"
              "Continue? [yN] ", stderr);
        int input = getchar();
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
    emu.console = aldo_nes_new(emu.debugger, emu.args->bcdsupport, tracelog);
    if (!emu.console) {
        perror("Unable to initialize console");
        result = EXIT_FAILURE;
        goto exit_trace;
    }
    if (!aldo_snapshot_extend(&emu.snapshot)) {
        perror("Unable to extend snapshot");
        result = EXIT_FAILURE;
        goto exit_console;
    }
    aldo_nes_powerup(emu.console, c, emu.args->zeroram);

    ui_loop *run_loop = setup_ui(&emu);
    int err = run_loop(&emu);
    if (err < 0) {
        fprintf(stderr, "UI run failure (%d): %s\n", err, aldo_ui_errstr(err));
        if (err == ALDO_UI_ERR_ERNO) {
            perror("UI system error");
        }
        result = EXIT_FAILURE;
    }
    dump_ram(&emu);
    aldo_nes_set_snapshot(emu.console, NULL);
    aldo_snapshot_cleanup(&emu.snapshot);
exit_console:
    if (aldo_nes_tracefailed(emu.console)) {
        fputs("Trace file I/O failure\n", stderr);
        result = EXIT_FAILURE;
    }
    aldo_nes_free(emu.console);
exit_trace:
    if (tracelog) {
        fclose(tracelog);
    }
exit_debug:
    aldo_debug_free(emu.debugger);
    return result;
}

static int run_cart(const struct cliargs *args, aldo_cart *c)
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

    aldo_cart *cart = load_cart(args->filepath);
    if (!cart) return EXIT_FAILURE;

    int result = run_cart(args, cart);
    aldo_cart_free(cart);
    return result;
}

//
// MARK: - Public Interface
//

int cli_run(int argc, char *argv[argc+1])
{
    struct cliargs args;
    if (!argparse_parse(&args, argc, argv)) return EXIT_FAILURE;

    int result = run_with_args(&args);
    argparse_cleanup(&args);
    return result;
}
