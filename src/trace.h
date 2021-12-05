//
//  trace.h
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#ifndef Aldo_trace_h
#define Aldo_trace_h

#include "cpu.h"

#include <stdint.h>
#include <stdio.h>

void trace_line(FILE *tracelog, uint64_t cycles, struct mos6502 *cpu);

#endif
