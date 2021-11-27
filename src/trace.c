//
//  trace.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#include "trace.h"

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

void trace_log(const struct traceline *line)
{
    static size_t lineno;

    if (!trace_enabled()) return;

    fprintf(Log, "%zu: CYC:%" PRIu64 "\n", ++lineno, line->cycles);
}
