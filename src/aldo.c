//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "control.h"
#include "ui.h"
#include "emu/cart.h"
#include "emu/nes.h"
#include "emu/snapshot.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static cart *load_cart(void)
{
    errno = 0;
    cart *program = NULL;
    FILE *const prgfile = fopen("a.out", "rb");
    if (!prgfile) {
        perror("Cannot open cart file");
        return program;
    }

    const int cart_result = cart_create(&program, prgfile);
    if (cart_result < 0) {
        fprintf(stderr, "Cart load failure: %s", cart_errstr(cart_result));
        if (cart_result == CART_IO_ERR) {
            perror("Cart IO error");
        }
    }

    return program;
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
    case '=':   // NOTE: "lowercase" +
        ++appstate->cycles_per_sec;
        goto pclamp_cps;
    case '+':
        appstate->cycles_per_sec += 10;
    pclamp_cps:
        if (appstate->cycles_per_sec > MaxCps) {
            appstate->cycles_per_sec = MaxCps;
        }
        break;
    case '-':
        --appstate->cycles_per_sec;
        goto nclamp_cps;
    case '_':   // NOTE: "uppercase" -
        appstate->cycles_per_sec -= 10;
    nclamp_cps:
        if (appstate->cycles_per_sec < MinCps) {
            appstate->cycles_per_sec = MinCps;
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
    const int cycles = nes_cycle(console, appstate->cyclebudget);
    appstate->cyclebudget -= cycles;
    appstate->total_cycles += cycles;
    nes_snapshot(console, snapshot);
    ui_refresh(appstate, snapshot);
}

//
// Public Interface
//

int aldo_run(int argc, char *argv[argc+1])
{
    puts("Aldo starting...");

    struct control appstate = {.cycles_per_sec = 4, .running = true};
    struct console_state snapshot;

    getchar();

    cart *c = load_cart();
    if (!c) {
        return EXIT_FAILURE;
    }

    nes *console = nes_new(c);
    // NOTE: console takes ownership of cart
    c = NULL;
    nes_powerup(console);
    // NOTE: initialize snapshot from console
    nes_snapshot(console, &snapshot);
    ui_init();

    do {
        ui_tick_start(&appstate, &snapshot);
        handle_input(&appstate, &snapshot, console);
        if (appstate.running) {
            update(&appstate, &snapshot, console);
        }
        ui_tick_end();
    } while (appstate.running);

    ui_cleanup();
    nes_free(console);
    console = NULL;
    snapshot.ram = snapshot.rom = NULL;

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}
