//
//  trace.c
//  Aldo
//
//  Created by Brandon Stansbury on 11/26/21.
//

#include "trace.h"

#include "bus.h"
#include "bytes.h"
#include "cpu.h"
#include "dis.h"
#include "ppu.h"
#include "snapshot.h"

#include <assert.h>
#include <inttypes.h>
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
                                  struct aldo_rp2c02 *ppu, aldo_debugger *dbg,
                                  const struct aldo_snapshot *snp)
{
    char peek[ALDO_DIS_PEEK_SIZE];
    int result = aldo_dis_peek(cpu, ppu, dbg, snp, peek);
    return fprintf(tracelog, " %s",
                   result < 0 ? aldo_dis_errstr(result) : peek);
}

static bool trace_registers(FILE *tracelog, const struct aldo_snapshot *snp)
{
    static const char flags[] = {
        'c', 'C', 'z', 'Z', 'i', 'I', 'd', 'D',
        'b', 'B', '-', '-', 'v', 'V', 'n', 'N',
    };

    int err = fprintf(tracelog, " A:%02X X:%02X Y:%02X P:%02X (",
                      snp->cpu.accumulator, snp->cpu.xindex, snp->cpu.yindex,
                      snp->cpu.status);
    if (err < 0) return false;
    for (size_t i = sizeof snp->cpu.status * 8; i > 0; --i) {
        size_t idx = i - 1;
        bool bit = aldo_getbit(snp->cpu.status, idx);
        if (fputc(flags[(idx * 2) + bit], tracelog) == EOF) return false;
    }
    return fprintf(tracelog, ") S:%02X", snp->cpu.stack_pointer) > 0;
}

//
// MARK: - Public Interface
//

bool aldo_trace_line(FILE *tracelog, int adjustment, uint64_t cycles,
                     struct aldo_mos6502 *cpu, struct aldo_rp2c02 *ppu,
                     aldo_debugger *dbg, const struct aldo_snapshot *snp)
{
    assert(tracelog != NULL);
    assert(cpu != NULL);
    assert(ppu != NULL);
    assert(dbg != NULL);
    assert(snp != NULL);

    // NOTE: does not include leading space in trace_registers
    static const int instw = 47;

    int written = trace_instruction(tracelog, cpu, snp);
    if (written < 0) return false;
    int peek = trace_instruction_peek(tracelog, cpu, ppu, dbg, snp);
    if (peek < 0) {
        return false;
    } else {
        written += peek;
    }
    int width = written <= instw ? instw - written : 0;
    assert(written <= instw);
    if (fprintf(tracelog, "%*s", width, "") < 0) return false;
    if (!trace_registers(tracelog, snp)) return false;
    struct aldo_ppu_coord p = aldo_ppu_trace(ppu, adjustment * Aldo_PpuRatio);
    return fprintf(tracelog, " PPU:%3d,%3d CPU:%" PRIu64 "\n", p.line,
                   p.dot, cycles + (uint64_t)adjustment) > 0;
}
