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

    uint8_t inst[3];
    const size_t instlen = bus_dma(line->cpubus, line->pc,
                                   sizeof inst / sizeof inst[0], inst);
    char disinst[DIS_INST_SIZE];
    const int result = dis_inst(line->pc, inst, instlen, disinst);
    if (result > 0) {
        // NOTE: nestest compatibility:
        //  - skip initial $
        //  - convert : to space
        //  - split line to skip two spaces between bytes and mnemonic
        disinst[5] = ' ';
        disinst[17] = '\0';
        fprintf(Log, "%s%s\t", disinst + 1, disinst + 19);
    } else {
        fprintf(Log, "%s\t", result < 0 ? dis_errstr(result) : "No inst");
    }

    fprintf(Log, "A:%02X X:%02X Y:%02X P:%02X SP:%02X\t", line->a, line->x,
            line->y, line->p, line->sp);
    fprintf(Log, "CYC:%" PRIu64 "\n", line->cycles);
}
