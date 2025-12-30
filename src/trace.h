//
//  trace.h
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#ifndef Aldo_trace_h
#define Aldo_trace_h

#include "debug.h"

#include <stdint.h>
#include <stdio.h>

struct aldo_mos6502;
struct aldo_rp2c02;
struct aldo_snapshot;

bool aldo_trace_line(FILE *tracelog, int adjustment, uint64_t cycles,
                     struct aldo_mos6502 *cpu, struct aldo_rp2c02 *ppu,
                     aldo_debugger *dbg, const struct aldo_snapshot *snp);

#endif
