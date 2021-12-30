//
//  debug.h
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#ifndef Aldo_debug_h
#define Aldo_debug_h

#include "control.h"
#include "cpu.h"
#include "snapshot.h"

typedef struct debugger_context debugctx;

debugctx *debug_new(const struct control *appstate);
void debug_free(debugctx *self);

void debug_attach_cpu(debugctx *self, struct mos6502 *cpu);
void debug_detach(debugctx *self);

void debug_override_reset(debugctx *self, uint16_t device_addr);
void debug_check(debugctx *self);

void debug_snapshot(debugctx *self, struct console_state *snapshot);

#endif
