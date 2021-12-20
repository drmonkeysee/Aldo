//
//  trace.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#include "trace.h"

#include "dis.h"

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>

static int trace_instruction(FILE *tracelog, const struct mos6502 *cpu,
                             const struct console_state *snapshot,
                             bool nestest)
{
    uint8_t inst[3];
    const size_t instlen = bus_dma(cpu->bus,
                                   snapshot->datapath.current_instruction,
                                   sizeof inst / sizeof inst[0], inst);
    char disinst[DIS_INST_SIZE];
    const int result = dis_inst(snapshot->datapath.current_instruction,
                                inst, instlen, disinst);
    if (result > 0) {
        if (nestest) {
            char *colon = strchr(disinst, ':');
            if (colon) {
                *colon = ' ';   // Replace ':' with ' '
            }
        }
        return fprintf(tracelog, "%s", disinst);
    } else {
        return fprintf(tracelog, "%s",
                       result < 0 ? dis_errstr(result) : "No inst");
    }
}

static int trace_instruction_peek(FILE *tracelog, struct mos6502 *cpu,
                                  const struct console_state *snapshot,
                                  bool nestest)
{
    char peek[DIS_PEEK_SIZE];
    const int result = dis_peek(snapshot->datapath.current_instruction, cpu,
                                snapshot, peek);
    return fprintf(tracelog, " %s", result < 0 ? dis_errstr(result) : peek);
}

static void trace_registers(FILE *tracelog,
                            const struct console_state *snapshot, bool nestest)
{
    static const char flags[] = {
        'c', 'C', 'z', 'Z', 'i', 'I', 'd', 'D',
        'b', 'B', '-', '-', 'v', 'V', 'n', 'N',
    };

    uint8_t status = snapshot->cpu.status;
    if (nestest) {
        status &= 0xef;  // Nestest always sets B low
    }
    fprintf(tracelog, " A:%02X X:%02X Y:%02X P:%02X",
            snapshot->cpu.accumulator, snapshot->cpu.xindex,
            snapshot->cpu.yindex, status);
    if (!nestest) {
        fputs(" (", tracelog);
        for (size_t i = sizeof snapshot->cpu.status * 8; i > 0; --i) {
            const size_t idx = i - 1;
            const bool bit = (snapshot->cpu.status >> idx) & 1;
            fputc(flags[(idx * 2) + bit], tracelog);
        }
        fputc(')', tracelog);
    }
    fprintf(tracelog, " SP:%02X", snapshot->cpu.stack_pointer);
}

//
// Public Interface
//

void trace_line(FILE *tracelog, uint64_t cycles, struct mos6502 *cpu,
                const struct console_state *snapshot, bool nestest)
{
    assert(cpu != NULL);
    assert(snapshot != NULL);

    if (!tracelog) return;

    // NOTE: does not include leading space in registers print-format
    static const int instw = 47;

    const int
        written = trace_instruction(tracelog, cpu, snapshot, nestest)
                    + trace_instruction_peek(tracelog, cpu, snapshot, nestest),
        width = written < 0 ? instw : (written > instw ? 0 : instw - written);
    fprintf(tracelog, "%*s", width, "");
    trace_registers(tracelog, snapshot, nestest);
    fprintf(tracelog, " CYC:%" PRIu64 "\n", cycles);
}
