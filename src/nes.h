//
//  nes.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#ifndef Aldo_nes_h
#define Aldo_nes_h

#include "cart.h"
#include "ctrlsignal.h"
#include "cycleclock.h"
#include "debug.h"
#include "snapshot.h"

#include <stdbool.h>

typedef struct nes_console nes;

#include "bridgeopen.h"
nes *nes_new(cart *c, debugctx *dbg, bool bcdsupport, bool zeroram, bool tron,
             bool dumpram);
void nes_free(nes *self);

void nes_mode(nes *self, enum csig_excmode mode);

void nes_powerup(nes *self);

void nes_ready(nes *self);
void nes_halt(nes *self);
void nes_interrupt(nes *self, enum csig_interrupt signal);
void nes_clear(nes *self, enum csig_interrupt signal);

void nes_cycle(nes *self, struct cycleclock *clock);

void nes_snapshot(nes *self, struct console_state *snapshot);
#include "bridgeclose.h"

#endif
