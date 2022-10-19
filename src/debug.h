//
//  debug.h
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#ifndef Aldo_debug_h
#define Aldo_debug_h

#include "bus.h"
#include "cpu.h"
#include "cycleclock.h"
#include "haltexpr.h"
#include "snapshot.h"

#include <stdint.h>

typedef struct debugger_context debugctx;

#include "bridgeopen.h"
debugctx *debug_new(int resetvector_override) aldo_nothrow;
void debug_free(debugctx *self) aldo_nothrow;

void debug_override_reset(debugctx *self, bus *b, uint16_t device_addr) aldo_nothrow;
void debug_addbreakpoint(debugctx *self, struct haltexpr expr) aldo_nothrow;
void debug_check(debugctx *self, const struct cycleclock *clk,
                 struct mos6502 *cpu) aldo_nothrow;
void debug_snapshot(debugctx *self, struct console_state *snapshot) aldo_nothrow;
#include "bridgeclose.h"

#endif
