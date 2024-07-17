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
#include <stddef.h>
#include <stdio.h>

typedef struct nes001 nes;

#include "bridgeopen.h"
br_libexport br_ownresult
nes *nes_new(debugger *dbg, bool bcdsupport, FILE *tracelog) br_nothrow;
br_libexport
void nes_free(nes *self) br_nothrow;

br_libexport
void nes_powerup(nes *self, cart *c, bool zeroram) br_nothrow;
br_libexport
void nes_powerdown(nes *self) br_nothrow;

// NOTE: RAM and VRAM are the same size so one accessor for both
br_libexport
size_t nes_ram_size(nes *self) br_nothrow;
br_libexport
bool nes_bcd_support(nes *self) br_nothrow;
br_libexport
enum csig_excmode nes_mode(nes *self) br_nothrow;
br_libexport
void nes_set_mode(nes *self, enum csig_excmode mode) br_nothrow;
br_libexport
void nes_ready(nes *self, bool ready) br_nothrow;
br_libexport
bool nes_probe(nes *self, enum csig_interrupt signal) br_nothrow;
br_libexport
void nes_set_probe(nes *self, enum csig_interrupt signal,
                   bool active) br_nothrow;

br_libexport
void nes_clock(nes *self, struct cycleclock *clock) br_nothrow;

br_libexport
void nes_snapshot(nes *self, struct snapshot *snp) br_nothrow;
br_libexport
void nes_dumpram(nes *self, FILE *f) br_nothrow;
#include "bridgeclose.h"

#endif
