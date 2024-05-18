//
//  trace.h
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#ifndef Aldo_trace_h
#define Aldo_trace_h

#include "cpu.h"
#include "debug.h"
#include "ppu.h"
#include "snapshot.h"

#include <stdint.h>
#include <stdio.h>

void trace_line(FILE *tracelog, uint64_t cycles, struct ppu_coord pixel,
                struct mos6502 *cpu, debugctx *dbg,
                const struct console_state *snapshot);

#endif
