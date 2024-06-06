//
//  trace.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#include "trace.h"

#include "bytes.h"
#include "dis.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

static int trace_instruction(FILE *tracelog, const struct mos6502 *cpu,
                             const struct console_state *snp)
{
    uint8_t bytes[3];
    const size_t instlen = bus_dma(cpu->mbus,
                                   snp->datapath.current_instruction,
                                   sizeof bytes / sizeof bytes[0], bytes);
    struct dis_instruction inst;
    int result = dis_parsemem_inst(instlen, bytes, 0, &inst);
    char disinst[DIS_INST_SIZE];
    if (result > 0) {
        result = dis_inst(snp->datapath.current_instruction, &inst, disinst);
    }
    return fprintf(tracelog, "%s",
                   result > 0 ? disinst : (result < 0
                                           ? dis_errstr(result)
                                           : "No inst"));
}

static int trace_instruction_peek(FILE *tracelog, struct mos6502 *cpu,
                                  debugger *dbg,
                                  const struct console_state *snp)
{
    char peek[DIS_PEEK_SIZE];
    const int result = dis_peek(snp->datapath.current_instruction, cpu, dbg,
                                snp, peek);
    return fprintf(tracelog, " %s", result < 0 ? dis_errstr(result) : peek);
}

static void trace_registers(FILE *tracelog, const struct console_state *snp)
{
    static const char flags[] = {
        'c', 'C', 'z', 'Z', 'i', 'I', 'd', 'D',
        'b', 'B', '-', '-', 'v', 'V', 'n', 'N',
    };

    fprintf(tracelog, " A:%02X X:%02X Y:%02X P:%02X (", snp->cpu.accumulator,
            snp->cpu.xindex, snp->cpu.yindex, snp->cpu.status);
    for (size_t i = sizeof snp->cpu.status * 8; i > 0; --i) {
        const size_t idx = i - 1;
        const bool bit = byte_getbit(snp->cpu.status, idx);
        fputc(flags[(idx * 2) + bit], tracelog);
    }
    fprintf(tracelog, ") S:%02X", snp->cpu.stack_pointer);
}

//
// Public Interface
//

void trace_line(FILE *tracelog, uint64_t cycles, struct ppu_coord pixel,
                struct mos6502 *cpu, debugger *dbg,
                const struct console_state *snp)
{
    assert(tracelog != NULL);
    assert(cpu != NULL);
    assert(dbg != NULL);
    assert(snp != NULL);

    // NOTE: does not include leading space in trace_registers
    static const int instw = 47;

    const int
        written = trace_instruction(tracelog, cpu, snp)
                    + trace_instruction_peek(tracelog, cpu, dbg, snp),
        width = written < 0 ? instw : (written > instw ? 0 : instw - written);
    assert(written <= instw);
    fprintf(tracelog, "%*s", width, "");
    trace_registers(tracelog, snp);
    fprintf(tracelog, " PPU:%3d,%3d CPU:%" PRIu64 "\n", pixel.line, pixel.dot,
            cycles);
}
