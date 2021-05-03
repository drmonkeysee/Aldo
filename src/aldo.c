//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "asm.h"
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
#include <string.h>

static const char
    *restrict const Version = "0.2.0", // TODO: autogenerate this

    *restrict const HelpOption = "--help",
    *restrict const HelpShortOption = "-h",
    *restrict const VersionOption = "--version",
    *restrict const VersionShortOption = "-V";

static const char DisassembleFlag = 'd',
                  VerboseFlag = 'v';

static void parse_args(struct control *appstate, int argc, char *argv[argc+1])
{
    appstate->me = argc > 0 && strlen(argv[0]) > 0 ? argv[0] : "aldo";
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const char *const arg = argv[i];
            if (strcmp(arg, HelpOption) == 0
                || strcmp(arg, HelpShortOption) == 0) {
                appstate->help = true;
            } else if (strcmp(arg, VersionOption) == 0
                       || strcmp(arg, VersionShortOption) == 0) {
                appstate->version = true;
            } else if (arg[0] == '-') {
                appstate->disassemble = appstate->disassemble
                                        || strchr(arg, DisassembleFlag);
                appstate->verbose = appstate->verbose
                                    || strchr(arg, VerboseFlag);
            } else {
                appstate->cartfile = arg;
            }
        }
    } else {
        appstate->help = true;
    }
}

static void print_usage(const struct control *appstate)
{
    printf("---=== Aldo Usage ===---\n");
    printf("%s [-dv] [command] file\n", appstate->me);
    printf("\noptions\n");
    printf("  -d\t: disassemble file, with -v will print duplicate lines\n");
    printf("  -v\t: verbose output\n");
    printf("\ncommands\n");
    printf("  -h\t: print usage (also --help)\n");
    printf("  -V\t: print version (also --version)\n");
    printf("\narguments\n");
    printf("  file\t: input file containing cartridge"
           " or program contents\n");
}

static void print_version(void)
{
    printf("Aldo %s", Version);
#ifdef __VERSION__
    printf(" (" __VERSION__ ")");
#endif
    printf("\n");
}

static cart *load_cart(const char *filename)
{
    cart *c = NULL;
    errno = 0;
    FILE *const prgfile = fopen(filename, "rb");
    if (!prgfile) {
        perror("Cannot open cart file");
        return c;
    }

    const int cart_result = cart_create(&c, prgfile);
    if (cart_result < 0) {
        fprintf(stderr, "Cart load failure (%d): %s\n",
                cart_result, cart_errstr(cart_result));
        if (cart_result == CART_IO_ERR) {
            perror("Cart IO error");
        }
    }

    return c;
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

static void emu_loop(struct control *appstate, cart *c)
{
    nes *console = nes_new(&c);
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
    nes_free(console);
    console = NULL;
    snapshot.ram = snapshot.rom = NULL;
}

//
// Public Interface
//

int aldo_run(int argc, char *argv[argc+1])
{
    struct control appstate = {.cycles_per_sec = 4, .running = true};
    parse_args(&appstate, argc, argv);

    if (appstate.help) {
        print_usage(&appstate);
        return EXIT_SUCCESS;
    }

    if (appstate.version) {
        print_version();
        return EXIT_SUCCESS;
    }

    if (!appstate.cartfile) {
        fprintf(stderr, "Error: no input file specified\n");
        print_usage(&appstate);
        return EXIT_FAILURE;
    }

    cart *const cart = load_cart(appstate.cartfile);
    if (!cart) {
        return EXIT_FAILURE;
    }

    if (appstate.disassemble) {
        return dis_cart(cart, appstate.verbose) == 0
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
    }

    emu_loop(&appstate, cart);

    return EXIT_SUCCESS;
}
