//
//  trace.h
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#ifndef Aldo_trace_h
#define Aldo_trace_h

#include "bus.h"

#include <stdbool.h>
#include <stdint.h>

struct traceline {
    bus *cpubus;    // Non-owning pointer
    uint64_t cycles;
    uint16_t pc;
    uint8_t a, p, sp, x, y;
};

bool trace_on(const char *logname);
void trace_off(void);

bool trace_enabled(void);

void trace_log(const struct traceline *line);

#endif
