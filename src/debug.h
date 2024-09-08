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

#include <stdbool.h>
#include <stddef.h>

struct breakpoint {
    struct haltexpr expr;
    bool enabled;
};

typedef struct debugger_context debugger;

#include "bridgeopen.h"
//
// MARK: - Export
//

br_libexport
extern const int NoResetVector;
br_libexport
extern const ptrdiff_t NoBreakpoint;

br_libexport br_ownresult
debugger *debug_new(void) br_nothrow;
br_libexport
void debug_free(debugger *self) br_nothrow;

br_libexport
int debug_vector_override(debugger *self) br_nothrow;
br_libexport
void debug_set_vector_override(debugger *self, int resetvector) br_nothrow;
br_libexport
void debug_bp_add(debugger *self, struct haltexpr expr) br_nothrow;
br_libexport
const struct breakpoint *debug_bp_at(debugger *self, ptrdiff_t at) br_nothrow;
br_libexport
void debug_bp_enable(debugger *self, ptrdiff_t at, bool enabled) br_nothrow;
br_libexport
const struct breakpoint *debug_halted(debugger *self) br_nothrow;
br_libexport
ptrdiff_t debug_halted_at(debugger *self) br_nothrow;
br_libexport
void debug_bp_remove(debugger *self, ptrdiff_t at) br_nothrow;
br_libexport
void debug_bp_clear(debugger *self) br_nothrow;
br_libexport
size_t debug_bp_count(debugger *self) br_nothrow;
br_libexport
void debug_reset(debugger *self) br_nothrow;

//
// MARK: - Internal
//

void debug_cpu_connect(debugger *self, struct mos6502 *cpu) br_nothrow;
void debug_cpu_disconnect(debugger *self) br_nothrow;
void debug_sync_bus(debugger *self) br_nothrow;
void debug_check(debugger *self, const struct cycleclock *clk) br_nothrow;
#include "bridgeclose.h"

#endif
