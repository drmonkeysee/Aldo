//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "argparse.h"
#include "cart.h"
#include "control.h"
#include "debug.h"
#include "dis.h"
#include "nes.h"
#include "ui.h"
#include "snapshot.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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
        if (err == CART_ERR_IO) {
            perror("Cart IO error");
        }
    }
    fclose(cartfile);

    return c;
}

static int print_cart_info(const struct control *appstate, cart *c)
{
    if (appstate->verbose) {
        puts("---=== Cart Info ===---");
    }
    printf("File\t\t: %s\n", ctrl_cartfilename(appstate->cartfile));
    cart_write_info(c, stdout, appstate->verbose);
    return EXIT_SUCCESS;
}

static int disassemble_cart_prg(const struct control *appstate, cart *c)
{
    return dis_cart_prg(c, appstate, stdout) == 0
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
}

static int decode_cart_chr(const struct control *appstate, cart *c)
{
    errno = 0;
    const int err = dis_cart_chr(c, appstate, stdout);
    if (err < 0) {
        fprintf(stderr, "CHR decode error (%d): %s\n", err,
                dis_errstr(err));
        if (err == DIS_ERR_IO) {
            perror("CHR decode file error");
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static void handle_input(struct control *appstate,
                         const struct console_state *snapshot, nes *console)
{
    const int c = ui_pollinput();
    switch (c) {
    case ' ':
        if (snapshot->lines.ready) {
            nes_halt(console);
        } else {
            nes_ready(console);
        }
        break;
    case '=':   // "Lowercase" +
        ++appstate->clock.cycles_per_sec;
        goto pclamp_cps;
    case '+':
        appstate->clock.cycles_per_sec += 10;
    pclamp_cps:
        if (appstate->clock.cycles_per_sec > MaxCps) {
            appstate->clock.cycles_per_sec = MaxCps;
        }
        break;
    case '-':
        --appstate->clock.cycles_per_sec;
        goto nclamp_cps;
    case '_':   // "Uppercase" -
        appstate->clock.cycles_per_sec -= 10;
    nclamp_cps:
        if (appstate->clock.cycles_per_sec < MinCps) {
            appstate->clock.cycles_per_sec = MinCps;
        }
        break;
    case 'i':
        if (snapshot->lines.irq) {
            nes_interrupt(console, NESI_IRQ);
        } else {
            nes_clear(console, NESI_IRQ);
        }
        break;
    case 'm':
        nes_mode(console, snapshot->mode + 1);
        break;
    case 'M':
        nes_mode(console, snapshot->mode - 1);
        break;
    case 'n':
        if (snapshot->lines.nmi) {
            nes_interrupt(console, NESI_NMI);
        } else {
            nes_clear(console, NESI_NMI);
        }
        break;
    case 'q':
        appstate->running = false;
        break;
    case 'r':
        appstate->ramsheet = (appstate->ramsheet + 1) % RamSheets;
        break;
    case 'R':
        --appstate->ramsheet;
        if (appstate->ramsheet < 0) {
            appstate->ramsheet = RamSheets - 1;
        }
        break;
    case 's':
        if (snapshot->lines.reset) {
            nes_interrupt(console, NESI_RES);
        } else {
            nes_clear(console, NESI_RES);
        }
        break;
    }
}

static void update(struct control *appstate, struct console_state *snapshot,
                   nes *console)
{
    nes_cycle(console, &appstate->clock);
    nes_snapshot(console, snapshot);
    ui_refresh(appstate, snapshot);
}

static int emu_loop(struct control *appstate, cart *c)
{
    int result = EXIT_SUCCESS;
    debugctx *dbg = debug_new(appstate);
    nes *console = nes_new(c, appstate->tron, dbg);
    if (!console) {
        result = EXIT_FAILURE;
        goto cleanup;
    };

    nes_powerup(console);
    // NOTE: initialize snapshot from console
    struct console_state snapshot;
    nes_snapshot(console, &snapshot);
    ui_init();

    do {
        ui_tick_start(appstate, &snapshot);
        handle_input(appstate, &snapshot, console);
        if (appstate->running) {
            update(appstate, &snapshot, console);
        }
        ui_tick_end();
    } while (appstate->running);

    ui_cleanup();
    snapshot.mem.prglength = 0;
    snapshot.mem.ram = NULL;
    nes_free(console);
    console = NULL;

cleanup:
    debug_free(dbg);
    dbg = NULL;
    return result;
}

static int run_cmd(struct control *appstate, cart *c)
{
    if (appstate->info) return print_cart_info(appstate, c);
    if (appstate->disassemble) return disassemble_cart_prg(appstate, c);
    if (appstate->chrdecode) return decode_cart_chr(appstate, c);
    return emu_loop(appstate, c);
}

//
// Public Interface
//

int aldo_run(int argc, char *argv[argc+1])
{
    struct control appstate;
    if (!argparse_parse(&appstate, argc, argv)) return EXIT_FAILURE;

    if (appstate.help) {
        argparse_usage(appstate.me);
        return EXIT_SUCCESS;
    }

    if (appstate.version) {
        argparse_version();
        return EXIT_SUCCESS;
    }

    if (!appstate.cartfile) {
        fputs("Error: no input file specified\n", stderr);
        argparse_usage(appstate.me);
        return EXIT_FAILURE;
    }

    cart *cart = load_cart(appstate.cartfile);
    if (!cart) {
        return EXIT_FAILURE;
    }

    const int result = run_cmd(&appstate, cart);
    cart_free(cart);
    cart = NULL;

    return result;
}
