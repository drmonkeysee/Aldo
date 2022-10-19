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
             bool dumpram) aldo_nothrow;
void nes_free(nes *self) aldo_nothrow;

void nes_mode(nes *self, enum csig_excmode mode) aldo_nothrow;

void nes_powerup(nes *self) aldo_nothrow;

void nes_ready(nes *self) aldo_nothrow;
void nes_halt(nes *self) aldo_nothrow;
void nes_interrupt(nes *self, enum csig_interrupt signal) aldo_nothrow;
void nes_clear(nes *self, enum csig_interrupt signal) aldo_nothrow;

void nes_cycle(nes *self, struct cycleclock *clock) aldo_nothrow;

void nes_snapshot(nes *self, struct console_state *snapshot) aldo_nothrow;
#include "bridgeclose.h"

#endif
