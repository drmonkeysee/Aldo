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
#include <stdio.h>

typedef struct nes_console nes;

#include "bridgeopen.h"
br_libexport br_ownresult
nes *nes_new(debugctx *dbg, bool bcdsupport, FILE *tracelog) br_nothrow;
br_libexport
void nes_free(nes *self) br_nothrow;

br_libexport
void nes_powerup(nes *self, cart *c, bool zeroram) br_nothrow;
br_libexport
void nes_powerdown(nes *self) br_nothrow;

br_libexport
enum csig_excmode nes_mode(nes *self) br_nothrow;
br_libexport
void nes_set_mode(nes *self, enum csig_excmode mode) br_nothrow;
br_libexport
void nes_ready(nes *self) br_nothrow;
br_libexport
void nes_halt(nes *self) br_nothrow;
br_libexport
void nes_interrupt(nes *self, enum csig_interrupt signal) br_nothrow;
br_libexport
void nes_clear(nes *self, enum csig_interrupt signal) br_nothrow;

br_libexport
void nes_cycle(nes *self, struct cycleclock *clock) br_nothrow;

br_libexport
void nes_snapshot(nes *self, struct console_state *snapshot) br_nothrow;
br_libexport
void nes_dumpram(nes *self, FILE *f) br_nothrow;
#include "bridgeclose.h"

#endif
