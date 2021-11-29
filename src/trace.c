//
//  trace.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#include "trace.h"

#include "dis.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

static FILE *restrict Log;

//
// Public Interface
//

bool trace_on(const char *logname)
{
    if (!trace_enabled()) {
        Log = fopen(logname, "w");
    }
    return trace_enabled();
}

void trace_off(void)
{
    if (!trace_enabled()) return;

    fclose(Log);
    Log = NULL;
}

bool trace_enabled(void)
{
    return Log != NULL;
}

// TODO: add nestest compatibility flag
void trace_log(const struct traceline *line)
{
    if (!trace_enabled()) return;

    // NOTE: does not include leading space in registers print-format
    static const int instw = 47;

    uint8_t inst[3];
    const size_t instlen = bus_dma(line->cpubus, line->pc,
                                   sizeof inst / sizeof inst[0], inst);
    char disinst[DIS_INST_SIZE];
    const int result = dis_inst(line->pc, inst, instlen, disinst);
    int written = 0;
    if (result > 0) {
        // NOTE: nestest compatibility:
        //  - convert : to space
        disinst[4] = ' ';
        written += fprintf(Log, "%s", disinst);
    } else {
        written += fprintf(Log, "%s",
                           result < 0 ? dis_errstr(result) : "No inst");
    }

    const int width = written < 0 ? instw : (written > instw
                                             ? 0
                                             : instw - written);
    fprintf(Log, "%*s", width, "");
    fprintf(Log, " A:%02X X:%02X Y:%02X P:%02X SP:%02X", line->a, line->x,
            line->y, line->p, line->sp);
    fprintf(Log, " CYC:%" PRIu64 "\n", line->cycles);
}
