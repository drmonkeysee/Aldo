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

    struct control appstate = {.cycles_per_sec = 4, .running = true};
    struct console_state snapshot;
    ui_init();

    const uint8_t test_prg[] = {0xea, 0xea, 0xea, 0x0, 0x42, 0x6, 0xea, 0x0, 0x6, 0x42, 0xea};
    nes *console = nes_new();
    // NOTE: initialize snapshot from console
    nes_snapshot(console, &snapshot);
    nes_powerup(console, sizeof test_prg, test_prg);
    // TODO: for now clock the cpu up to the first non-RESET cycle then halt
    nes_mode(console, EXC_STEP);
    nes_ready(console);
    appstate.total_cycles += nes_clock(console);
    nes_mode(console, snapshot.mode);

    do {
        ui_tick_start(&appstate, &snapshot);
        const int c = ui_pollinput();
        switch (c) {
        case ' ':
            if (snapshot.lines.ready) {
                nes_halt(console);
            } else {
                nes_ready(console);
            }
            break;
        case 'm':
            nes_mode(console, snapshot.mode + 1);
            break;
        case 'M':
            nes_mode(console, snapshot.mode - 1);
            break;
        case 'n':
            appstate.ramsheet = (appstate.ramsheet + 1) % RamSheets;
            break;
        case 'p':
            --appstate.ramsheet;
            if (appstate.ramsheet < 0) {
                appstate.ramsheet = RamSheets - 1;
            }
            break;
        case 'q':
            appstate.running = false;
            break;
        }
        if (appstate.running) {
            const int cycles = nes_cycle(console, appstate.cyclebudget);
            appstate.cyclebudget -= cycles;
            appstate.total_cycles += cycles;
            nes_snapshot(console, &snapshot);
            ui_refresh(&appstate, &snapshot);
        }
        ui_tick_end();
    } while (appstate.running);

    ui_cleanup();
    nes_free(console);
    console = NULL;

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}
