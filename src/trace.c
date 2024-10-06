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

static int trace_instruction(FILE *tracelog, const struct aldo_mos6502 *cpu,
                             const struct aldo_snapshot *snp)
{
    uint8_t bytes[3];
    size_t instlen = aldo_bus_copy(cpu->mbus,
                                   snp->cpu.datapath.current_instruction,
                                   sizeof bytes / sizeof bytes[0], bytes);
    struct aldo_dis_instruction inst;
    int result = aldo_dis_parsemem_inst(instlen, bytes, 0, &inst);
    char disinst[ALDO_DIS_INST_SIZE];
    if (result > 0) {
        result = aldo_dis_inst(snp->cpu.datapath.current_instruction, &inst,
                               disinst);
    }
    return fprintf(tracelog, "%s",
                   result > 0 ? disinst : (result < 0
                                           ? aldo_dis_errstr(result)
                                           : "No inst"));
}

static int trace_instruction_peek(FILE *tracelog, struct aldo_mos6502 *cpu,
                                  aldo_debugger *dbg,
                                  const struct aldo_snapshot *snp)
{
    char peek[ALDO_DIS_PEEK_SIZE];
    int result = aldo_dis_peek(snp->cpu.datapath.current_instruction, cpu, dbg,
                               snp, peek);
    return fprintf(tracelog, " %s",
                   result < 0 ? aldo_dis_errstr(result) : peek);
}

static void trace_registers(FILE *tracelog, const struct aldo_snapshot *snp)
{
    static const char flags[] = {
        'c', 'C', 'z', 'Z', 'i', 'I', 'd', 'D',
        'b', 'B', '-', '-', 'v', 'V', 'n', 'N',
    };

    fprintf(tracelog, " A:%02X X:%02X Y:%02X P:%02X (", snp->cpu.accumulator,
            snp->cpu.xindex, snp->cpu.yindex, snp->cpu.status);
    for (size_t i = sizeof snp->cpu.status * 8; i > 0; --i) {
        size_t idx = i - 1;
        bool bit = aldo_byte_getbit(snp->cpu.status, idx);
        fputc(flags[(idx * 2) + bit], tracelog);
    }
    fprintf(tracelog, ") S:%02X", snp->cpu.stack_pointer);
}

//
// MARK: - Public Interface
//

void aldo_trace_line(FILE *tracelog, uint64_t cycles,
                     struct aldo_ppu_coord pixel, struct aldo_mos6502 *cpu,
                     aldo_debugger *dbg, const struct aldo_snapshot *snp)
{
    assert(tracelog != NULL);
    assert(cpu != NULL);
    assert(dbg != NULL);
    assert(snp != NULL);

    // NOTE: does not include leading space in trace_registers
    static const int instw = 47;

    int written = trace_instruction(tracelog, cpu, snp)
                    + trace_instruction_peek(tracelog, cpu, dbg, snp),
        width = written < 0 ? instw : (written > instw ? 0 : instw - written);
    assert(written <= instw);
    fprintf(tracelog, "%*s", width, "");
    trace_registers(tracelog, snp);
    fprintf(tracelog, " PPU:%3d,%3d CPU:%" PRIu64 "\n", pixel.line, pixel.dot,
            cycles);
}
