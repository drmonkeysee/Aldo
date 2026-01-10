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
#include "debug.h"

#include <stddef.h>
#include <stdio.h>

struct aldo_clock;
struct aldo_snapshot;
typedef struct aldo_nes001 aldo_nes;

#include "bridgeopen.h"
// NOTE: if returns null then errno is set due to failed allocation
aldo_export aldo_ownresult
aldo_nes *aldo_nes_new(aldo_debugger *dbg, bool bcdsupport,
                       FILE *tracelog) aldo_nothrow;
aldo_export
void aldo_nes_free(aldo_nes *self) aldo_nothrow;

aldo_export
void aldo_nes_powerup(aldo_nes *self, aldo_cart *c, bool zeroram) aldo_nothrow;
aldo_export
void aldo_nes_powerdown(aldo_nes *self) aldo_nothrow;

aldo_export
int aldo_nes_max_tcpu() aldo_nothrow;
// NOTE: RAM and VRAM are the same size so one accessor for both
aldo_export
size_t aldo_nes_ram_size(aldo_nes *self) aldo_nothrow;
aldo_export
void aldo_nes_screen_size(int *width, int *height) aldo_nothrow;
aldo_export
bool aldo_nes_bcd_support(aldo_nes *self) aldo_nothrow;
aldo_export
bool aldo_nes_tracefailed(aldo_nes *self) aldo_nothrow;
aldo_export
enum aldo_execmode aldo_nes_mode(aldo_nes *self) aldo_nothrow;
aldo_export
void aldo_nes_set_mode(aldo_nes *self, enum aldo_execmode mode) aldo_nothrow;
aldo_export
void aldo_nes_ready(aldo_nes *self, bool ready) aldo_nothrow;
aldo_export
bool aldo_nes_probe(aldo_nes *self, enum aldo_interrupt signal) aldo_nothrow;
aldo_export
void aldo_nes_set_probe(aldo_nes *self, enum aldo_interrupt signal,
                        bool active) aldo_nothrow;

aldo_export
void aldo_nes_clock(aldo_nes *self, struct aldo_clock *clock) aldo_nothrow;
aldo_export
int aldo_nes_cycle_factor() aldo_nothrow;
aldo_export
int aldo_nes_frame_factor() aldo_nothrow;

aldo_export
void aldo_nes_set_snapshot(aldo_nes *self, struct aldo_snapshot *snp) aldo_nothrow;
aldo_export
void aldo_nes_dumpram(aldo_nes *self, FILE *fs[aldo_cz(3)],
                      bool errs[aldo_cz(3)]) aldo_nothrow;
#include "bridgeclose.h"

#endif
