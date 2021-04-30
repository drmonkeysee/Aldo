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

int aldo_run(void)
{
    puts("Aldo starting...");

    struct control appstate = {.cycles_per_sec = 4, .running = true};
    struct console_state snapshot;
    ui_init();

    const uint8_t test_prg[] = {
        0x58, // cli
        0x4c, 0xb, 0x80,   // jmp $800B
        0xe6, 0x90, // inc zp,$90
        0x40,   // rti
        0x18,   // clc
        0x69, 0x5, // adc 5
        0x60, // rts
        0xa9, 0xa, // lda $2
        0x20, 0x7, 0x80, // jsr $8007
        0x48,
        0x68,
        0x8,
        0xa9, 0x0,
        0x8d, 0xfd, 0x1,
        0x28,
        0x8,
        0x85, 0xff, // sta $5
        0xa9, 0x80, // lda $80
        0x85, 0x0, // sta $6
        0xa2, 0x5,  // ldx #5
        0xa9, 0x88, // lda $88
        0x6a,           // ror
        0xca,
        0xd0, 0xfc, // bne -4
        //0x4c, 0xa, 0x80, // jmp $800a
        //0x6c, 0xff, 0x0, // jmp ($00FF)
        0xc6, 0x24,    // DEC $24
        0xe6, 0x26,    // inc $24
        0xa9, 0xa, // lda #$10
        0x85, 0x20, // sta $20
        0xa5, 0x25, // lda $25
        0xa2, 0x5, // ldx, #$2
        0x95, 0x30, // sta $30,X
        0xa0, 0x3, // ldy #$3
        0xb9, 0x1, 0x3, // lda $03ff,Y
        0xb9, 0xff, 0x3, // lda $03ff,Y
        0xb1, 0x16, // lda $16,Y
        0xa0, 0xd6, //ldy #$d6
        0xb1, 0x16, // lda $16,Y
        0x99, 0x2, 0x3, // sta $0302,Y
        0x99, 0xff, 0x2, // sta $03ff,Y
    };
    nes *console = nes_new();
    nes_powerup(console, sizeof test_prg, test_prg);
    // NOTE: initialize snapshot from console
    nes_snapshot(console, &snapshot);

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
