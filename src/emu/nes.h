//
//  nes.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#ifndef Aldo_emu_nes_h
#define Aldo_emu_nes_h

#include "cart.h"
#include "snapshot.h"

#include <stddef.h>
#include <stdint.h>

enum nes_interrupt {
    NESI_IRQ,
    NESI_NMI,
    NESI_RES,
};
typedef struct nes_console nes;

// NOTE: nes_new takes ownership of *c and sets it to null
nes *nes_new(cart **c);
void nes_free(nes *self);

void nes_mode(nes *self, enum nexcmode mode);

void nes_powerup(nes *self);

void nes_ready(nes *self);
void nes_halt(nes *self);
void nes_interrupt(nes *self, enum nes_interrupt signal);
void nes_clear(nes *self, enum nes_interrupt signal);

int nes_cycle(nes *self, int cpubudget);
int nes_clock(nes *self);

void nes_snapshot(nes *self, struct console_state *snapshot);

#endif
