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

debugctx *debug_new(int resetvector_override);
void debug_free(debugctx *self);

void debug_override_reset(debugctx *self, bus *b, uint16_t device_addr);
void debug_addbreakpoint(debugctx *self, struct haltexpr expr);
void debug_check(debugctx *self, const struct cycleclock *clk,
                 struct mos6502 *cpu);
void debug_snapshot(debugctx *self, struct console_state *snapshot);

#endif
