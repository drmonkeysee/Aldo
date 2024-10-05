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

typedef struct aldo_nes001 aldo_nes;

#include "bridgeopen.h"
br_libexport br_ownresult
aldo_nes *aldo_nes_new(aldo_debugger *dbg, bool bcdsupport,
                       FILE *tracelog) br_nothrow;
br_libexport
void aldo_nes_free(aldo_nes *self) br_nothrow;

br_libexport
void aldo_nes_powerup(aldo_nes *self, cart *c, bool zeroram) br_nothrow;
br_libexport
void aldo_nes_powerdown(aldo_nes *self) br_nothrow;

// NOTE: RAM and VRAM are the same size so one accessor for both
br_libexport
size_t aldo_nes_ram_size(aldo_nes *self) br_nothrow;
br_libexport
bool aldo_nes_bcd_support(aldo_nes *self) br_nothrow;
br_libexport
enum csig_excmode aldo_nes_mode(aldo_nes *self) br_nothrow;
br_libexport
void aldo_nes_set_mode(aldo_nes *self, enum csig_excmode mode) br_nothrow;
br_libexport
void aldo_nes_ready(aldo_nes *self, bool ready) br_nothrow;
br_libexport
bool aldo_nes_probe(aldo_nes *self, enum csig_interrupt signal) br_nothrow;
br_libexport
void aldo_nes_set_probe(aldo_nes *self, enum csig_interrupt signal,
                        bool active) br_nothrow;

br_libexport
void aldo_nes_clock(aldo_nes *self, struct cycleclock *clock) br_nothrow;
br_libexport
int aldo_nes_cycle_factor(void) br_nothrow;
br_libexport
int aldo_nes_frame_factor(void) br_nothrow;

br_libexport
void aldo_nes_snapshot(aldo_nes *self, struct aldo_snapshot *snp) br_nothrow;
br_libexport
void aldo_nes_dumpram(aldo_nes *self, FILE *fs[br_csz(3)]) br_nothrow;
#include "bridgeclose.h"

#endif
