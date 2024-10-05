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

void aldo_trace_line(FILE *tracelog, uint64_t cycles,
                     struct aldo_ppu_coord pixel, struct mos6502 *cpu,
                     aldo_debugger *dbg, const struct aldo_snapshot *snp);

#endif
