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

// X(symbol, value, error string)
#define NES_ERRCODE_X \
X(NES_ERR_ERNO, -1, "SYSTEM ERROR")

enum {
#define X(s, v, e) s = v,
    NES_ERRCODE_X
#undef X
};

typedef struct nes_console nes;

#include "bridgeopen.h"
br_libexport
const char *nes_errstr(int err) br_nothrow;

br_libexport br_ownresult
nes *nes_new(debugctx *dbg, bool bcdsupport, bool tron) br_nothrow;
br_libexport
void nes_free(nes *self) br_nothrow;

br_libexport
void nes_powerup(nes *self, cart *c, bool zeroram) br_nothrow;

br_libexport
void nes_mode(nes *self, enum csig_excmode mode) br_nothrow;
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
br_libexport br_checkerror
int nes_dumpram(nes *self, const char *br_noalias filepath) br_nothrow;
#include "bridgeclose.h"

#endif
