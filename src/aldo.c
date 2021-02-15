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
    do {
        const int c = ui_pollinput();
        switch (c) {
        case ' ':
            appstate.total_cycles += nes_cycle(console);
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
