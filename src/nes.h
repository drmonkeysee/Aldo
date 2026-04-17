//
//  nes.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#ifndef Aldo_nes_h
#define Aldo_nes_h

#include "console.h"

#include <stddef.h>
#include <stdio.h>

struct aldo_clock;
typedef struct aldo_nes001 aldo_nes;

extern const size_t Aldo_NesSize;

bool aldo_nes_init(aldo_nes *self);
void aldo_nes_powerup(aldo_nes *self, bool zeroram);

void aldo_nes_cart_connect(aldo_console *self);
void aldo_nes_cart_disconnect(aldo_console *self);
void aldo_nes_cleanup(aldo_console *self);

bool aldo_nes_clock(aldo_nes *self, struct aldo_clock *clock);
void aldo_nes_snapshot_init(aldo_nes *self);
void aldo_nes_snapshot_core(aldo_nes *self);

void aldo_nes_dumpram(aldo_nes *self, FILE *fs[static 3], bool errs[static 3]);

#endif
