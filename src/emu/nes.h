//
//  nes.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#ifndef Aldo_emu_nes_h
#define Aldo_emu_nes_h

#include "snapshot.h"

#include <stddef.h>
#include <stdint.h>

typedef struct nes_console nes;

nes *nes_new(void);
void nes_free(nes *self);

void nes_powerup(nes *self, size_t sz, uint8_t prog[restrict sz]);
int nes_step(nes *self);

void nes_snapshot(nes *self, struct console_state *restrict snapshot);

#endif
