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
    Log = fopen(logname, "w");
    return Log;
}

void trace_off(void)
{
    fclose(Log);
    Log = NULL;
}

void trace_log(const struct traceline *line)
{
    static size_t lineno;
    fprintf(Log, "%zu: CYC:%" PRIu64 "\n", ++lineno, line->cycles);
}
