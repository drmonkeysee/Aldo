//
//  debug.h
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#ifndef Aldo_debug_h
#define Aldo_debug_h

#include "haltexpr.h"

#include <stddef.h>

struct aldo_clock;
struct aldo_mos6502;

struct aldo_breakpoint {
    struct aldo_haltexpr expr;
    bool enabled;
};

typedef struct aldo_debugger_context aldo_debugger;

#include "bridgeopen.h"
//
// MARK: - Export
//

aldo_export
extern const int Aldo_NoResetVector;
aldo_export
extern const ptrdiff_t Aldo_NoBreakpoint;

// if returns null then errno is set due to failed allocation
aldo_export aldo_ownresult
aldo_debugger *aldo_debug_new() aldo_nothrow;
aldo_export
void aldo_debug_free(aldo_debugger *self) aldo_nothrow;

aldo_export
int aldo_debug_vector_override(aldo_debugger *self) aldo_nothrow;
aldo_export
void aldo_debug_set_vector_override(aldo_debugger *self,
                                    int resetvector) aldo_nothrow;
// if returns false then errno is set due to failed allocation
aldo_export aldo_checkerr
bool aldo_debug_bp_add(aldo_debugger *self,
                       struct aldo_haltexpr expr) aldo_nothrow;
aldo_export
const struct aldo_breakpoint *aldo_debug_bp_at(aldo_debugger *self,
                                               ptrdiff_t at) aldo_nothrow;
aldo_export
void aldo_debug_bp_enable(aldo_debugger *self, ptrdiff_t at,
                          bool enabled) aldo_nothrow;
aldo_export
const struct aldo_breakpoint *
aldo_debug_halted(aldo_debugger *self) aldo_nothrow;
aldo_export
ptrdiff_t aldo_debug_halted_at(aldo_debugger *self) aldo_nothrow;
aldo_export
void aldo_debug_bp_remove(aldo_debugger *self, ptrdiff_t at) aldo_nothrow;
aldo_export
void aldo_debug_bp_clear(aldo_debugger *self) aldo_nothrow;
aldo_export
size_t aldo_debug_bp_count(aldo_debugger *self) aldo_nothrow;
aldo_export
void aldo_debug_reset(aldo_debugger *self) aldo_nothrow;

//
// MARK: - Internal
//

void aldo_debug_cpu_connect(aldo_debugger *self,
                            struct aldo_mos6502 *cpu) aldo_nothrow;
void aldo_debug_cpu_disconnect(aldo_debugger *self) aldo_nothrow;
void aldo_debug_sync_bus(aldo_debugger *self) aldo_nothrow;
void aldo_debug_check(aldo_debugger *self,
                      const struct aldo_clock *clk) aldo_nothrow;
#include "bridgeclose.h"

#endif
