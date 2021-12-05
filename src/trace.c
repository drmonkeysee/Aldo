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

// TODO: add nestest compatibility flag
void trace_line(FILE *tracelog, uint64_t cycles, struct mos6502 *cpu)
{
    if (!tracelog) return;

    // NOTE: does not include leading space in registers print-format
    static const int instw = 47;

    uint16_t pc;
    uint8_t inst[3], cpustatus;
    // NOTE: defer to the cpu for ambiguous state values
    cpu_traceline(cpu, &pc, &cpustatus);
    const size_t instlen = bus_dma(cpu->bus, pc,
                                   sizeof inst / sizeof inst[0], inst);

    char disinst[DIS_INST_SIZE];
    int result = dis_inst(pc, inst, instlen, disinst), written = 0;
    if (result > 0) {
        // NOTE: nestest compatibility:
        //  - convert : to space
        disinst[4] = ' ';
        written += fprintf(tracelog, "%s", disinst);
    } else {
        written += fprintf(tracelog, "%s",
                           result < 0 ? dis_errstr(result) : "No inst");
    }

    char peek[DIS_PEEK_SIZE];
    result = dis_peek(pc, cpu, peek);
    written += fprintf(tracelog, " %s",
                       result < 0 ? dis_errstr(result) : peek);

    const int width = written < 0 ? instw : (written > instw
                                             ? 0
                                             : instw - written);
    fprintf(tracelog, "%*s", width, "");
    fprintf(tracelog, " A:%02X X:%02X Y:%02X P:%02X SP:%02X", cpu->a, cpu->x,
            cpu->y, cpustatus, cpu->s);
    fprintf(tracelog, " CYC:%" PRIu64 "\n", cycles);
}
