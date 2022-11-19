//
//  debug.h
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#ifndef Aldo_debug_h
#define Aldo_debug_h

#include "cpu.h"
#include "cycleclock.h"
#include "haltexpr.h"
#include "snapshot.h"

typedef struct debugger_context debugctx;

#include "bridgeopen.h"
//
// Export
//

br_libexport
extern const int NoResetVector;

br_libexport br_ownresult
debugctx *debug_new(void) br_nothrow;
br_libexport
void debug_free(debugctx *self) br_nothrow;

br_libexport
void debug_addbreakpoint(debugctx *self, struct haltexpr expr) br_nothrow;
br_libexport
void debug_set_reset(debugctx *self, int resetvector) br_nothrow;

//
// Internal
//

void debug_cpu_connect(debugctx *self, struct mos6502 *cpu) br_nothrow;
void debug_cpu_disconnect(debugctx *self) br_nothrow;
void debug_override_reset(debugctx *self) br_nothrow;
void debug_check(debugctx *self, const struct cycleclock *clk) br_nothrow;
void debug_snapshot(debugctx *self, struct console_state *snapshot) br_nothrow;
#include "bridgeclose.h"

#endif
