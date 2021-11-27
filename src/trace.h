//
//  trace.h
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#ifndef Aldo_trace_h
#define Aldo_trace_h

#include <stdbool.h>
#include <stdint.h>

struct traceline {
    uint64_t cycles;
    uint16_t pc;
    uint8_t a, inst[3], p, sp, x, y;
};

bool trace_on(const char *logname);
void trace_off(void);

void trace_log(const struct traceline *line);

#endif
