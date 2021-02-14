//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

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
    bool running = true;
    uint64_t total_cycles = 0;
    struct console_state snapshot;
    const uint8_t test_prg[] = {0xea, 0xea, 0xea, 0x0, 0x42, 0x6, 0xea, 0xea};
    nes_powerup(console, sizeof test_prg, test_prg);
    do {
        const int c = ui_pollinput();
        switch (c) {
        case ' ':
            total_cycles += nes_cycle(console);
            break;
        case 'b':
            ui_ram_prev();
            break;
        case 'n':
            ui_ram_next();
            break;
        case 'q':
            running = false;
            break;
        }
        if (running) {
            nes_snapshot(console, &snapshot);
            ui_refresh(&snapshot, total_cycles);
        }
    } while (running);
    ui_cleanup();

    nes_free(console);
    console = NULL;

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}
