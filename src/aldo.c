//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "control.h"
#include "ui.h"
#include "emu/nes.h"
#include "emu/snapshot.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int aldo_run(void)
{
    puts("Aldo starting...");

    nes *console = nes_new();

    ui_init();
    struct control appstate = {.running = true};
    struct console_state snapshot;
    const uint8_t test_prg[] = {0xea, 0xea, 0xea, 0x0, 0x42, 0x6, 0xea, 0xea};
    nes_powerup(console, sizeof test_prg, test_prg);
    // TODO: for now clock the cpu up to the first non-RESET cycle then halt
    appstate.total_cycles += nes_step(console);

    do {
        const int c = ui_pollinput();
        switch (c) {
        case ' ':
            switch (appstate.exec_mode) {
            case EXC_CYCLE:
                appstate.total_cycles += nes_cycle(console);
                break;
            case EXC_STEP:
                appstate.total_cycles += nes_step(console);
                break;
            case EXC_RUN:
                appstate.total_cycles += nes_clock(console);
                break;
            default:
                assert(((void)"INVALID EXC MODE", false));
            }
            break;
        case 'b':
            ctl_ram_prev(&appstate);
            break;
        case 'm':
            ctl_toggle_excmode(&appstate);
            break;
        case 'n':
            ctl_ram_next(&appstate);
            break;
        case 'q':
            appstate.running = false;
            break;
        }
        if (appstate.running) {
            nes_snapshot(console, &snapshot);
            ui_refresh(&appstate, &snapshot);
        }
    } while (appstate.running);

    ui_cleanup();
    nes_free(console);
    console = NULL;

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}
