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
    case 'm':
        nes_mode(console, snapshot->mode + 1);
        break;
    case 'M':
        nes_mode(console, snapshot->mode - 1);
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
    case 'q':
        appstate->running = false;
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

int aldo_run(void)
{
    puts("Aldo starting...");

    struct control appstate = {.cycles_per_sec = 4, .running = true};
    struct console_state snapshot;
    ui_init();

    const uint8_t test_prg[] = {
        0xa9, 0xa, // lda #$10
        0x85, 0x20, // sta $20
        0xa5, 0x25, // lda $25
        0xa2, 0x5, // ldx, #$2
        0x95, 0x30, // sta $30,X
        0xa0, 0x3, // ldy #$3
        0xb9, 0x1, 0x3, // lda $03ff,Y
        0xb9, 0xff, 0x3, // lda $03ff,Y
        0xa0, 0xd6, //ldy #$d6
        0xb1, 0x16, // lda $16,Y
        0x99, 0x2, 0x3, // sta $0302,Y
        0x99, 0xff, 0x2, // sta $03ff,Y
    };
    nes *console = nes_new();
    nes_powerup(console, sizeof test_prg, test_prg);
    // NOTE: initialize snapshot from console
    nes_snapshot(console, &snapshot);
    // TODO: for now clock the cpu up to the first non-RESET cycle then halt
    nes_mode(console, NEXC_STEP);
    nes_ready(console);
    appstate.total_cycles += nes_clock(console);
    nes_mode(console, snapshot.mode);

    do {
        ui_tick_start(&appstate, &snapshot);
        handle_input(&appstate, &snapshot, console);
        if (appstate.running) {
            update(&appstate, &snapshot, console);
        }
        ui_tick_end();
    } while (appstate.running);

    nes_free(console);
    console = NULL;
    snapshot.ram = snapshot.rom = NULL;
    ui_cleanup();

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}
