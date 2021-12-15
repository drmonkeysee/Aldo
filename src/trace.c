//
//  trace.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#include "trace.h"

#include "dis.h"
#include "snapshot.h"

#include <inttypes.h>
#include <stddef.h>

// TODO: add nestest compatibility flag
void trace_line(FILE *tracelog, uint64_t cycles, struct mos6502 *cpu)
{
    if (!tracelog) return;

    // NOTE: does not include leading space in registers print-format
    static const int instw = 47;

    uint8_t inst[3];
    struct console_state snapshot;
    cpu_snapshot(cpu, &snapshot);
    const size_t instlen = bus_dma(cpu->bus,
                                   snapshot.datapath.current_instruction,
                                   sizeof inst / sizeof inst[0], inst);

    char disinst[DIS_INST_SIZE];
    int result = dis_inst(snapshot.datapath.current_instruction, inst, instlen,
                          disinst),
        written = 0;
    if (result > 0) {
        // NOTE: nestest compatibility:
        //  - convert : to space
        //disinst[4] = ' ';
        written += fprintf(tracelog, "%s", disinst);
    } else {
        written += fprintf(tracelog, "%s",
                           result < 0 ? dis_errstr(result) : "No inst");
    }

    char peek[DIS_PEEK_SIZE];
    result = dis_peek(snapshot.datapath.current_instruction, cpu, &snapshot,
                      peek);
    written += fprintf(tracelog, " %s",
                       result < 0 ? dis_errstr(result) : peek);

    const int width = written < 0 ? instw : (written > instw
                                             ? 0
                                             : instw - written);
    fprintf(tracelog, "%*s", width, "");
    fprintf(tracelog, " A:%02X X:%02X Y:%02X P:%02X (",
            snapshot.cpu.accumulator, snapshot.cpu.xindex, snapshot.cpu.yindex,
            snapshot.cpu.status);
    static const char flags[] = {
        'c', 'C', 'z', 'Z', 'i', 'I', 'd', 'D',
        'b', 'B', '-', '-', 'v', 'V', 'n', 'N',
    };
    for (size_t i = sizeof snapshot.cpu.status * 8; i > 0; --i) {
        const size_t idx = i - 1;
        const bool bit = (snapshot.cpu.status >> idx) & 1;
        fputc(flags[(idx * 2) + bit], tracelog);
    }
    fprintf(tracelog, ") SP:%02X", snapshot.cpu.stack_pointer);
    fprintf(tracelog, " CYC:%" PRIu64 "\n", cycles);
}
