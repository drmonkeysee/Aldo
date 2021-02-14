//
//  asm.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#include "asm.h"

#include "emu/bytes.h"
#include "emu/decode.h"

#include <assert.h>
#include <stdio.h>

static const char *restrict const Mnemonics[] = {
#define X(s) #s,
    DEC_INST_X
#undef X
};

static const int InstLens[] = {
#define X(s, b, ...) b,
    DEC_ADDRMODE_X
#undef X
};

#define X(s, b, ...) static const char *restrict const s##_StringTable[] = { \
    __VA_ARGS__ \
};
    DEC_ADDRMODE_X
#undef X

static const char *restrict const *const StringTables[] = {
#define X(s, b, ...) s##_StringTable,
    DEC_ADDRMODE_X
#undef X
};

int dis_inst(uint16_t addr, const uint8_t *dispc, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE])
{
    assert(dispc != NULL);
    assert(bytesleft >= 0);
    assert(dis != NULL);

    if (bytesleft == 0) return 0;

    const uint8_t opcode = *dispc;
    const struct decoded dec = Decode[opcode];
    const int instlen = InstLens[dec.mode];
    if (bytesleft < instlen) return DIS_EOF;

    int count;
    unsigned int total;
    total = count = sprintf(dis, "$%04X: ", addr);
    if (count < 0) return DIS_FMT_FAIL;

    for (int i = 0; i < instlen; ++i) {
        count = sprintf(dis + total, "%02X ", *(dispc + i));
        if (count < 0) return DIS_FMT_FAIL;
        total += count;
    }
    // Padding between raw bytes and disassembled instruction
    count = sprintf(dis + total, "%*s", (4 - instlen) * 3, "");
    if (count < 0) return DIS_FMT_FAIL;
    total += count;

    count = sprintf(dis + total, "%s ", Mnemonics[dec.instruction]);
    if (count < 0) return DIS_FMT_FAIL;
    total += count;

    const char *restrict const *const strtable = StringTables[dec.mode];
    switch (instlen) {
    case 1:
        count = sprintf(dis + total, "%s", strtable[1]);
        break;
    case 2:
        count = sprintf(dis + total, strtable[1], *(dispc + 1));
        break;
    case 3:
        count = sprintf(dis + total, strtable[2], batowr(dispc + 1));
        break;
    default:
        return DIS_INV_ADDRM;
    }
    if (count < 0) return DIS_FMT_FAIL;
    total += count;

    assert(total < DIS_INST_SIZE);
    return instlen;
}

const char *dis_errstr(int error)
{
    switch (error) {
#define X(s, v, e) case DIS_##s: return e;
    DIS_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_MNEM_SIZE])
{
    assert(dis != NULL);

    // NOTE: stop updating datapath display after max possible bytes
    // have been read for instruction-decoding
    if (snapshot->cpu.exec_cycle > 2) return 0;

    const struct decoded dec = Decode[snapshot->cpu.opcode];
    int count = sprintf(dis, "%s ", Mnemonics[dec.instruction]);
    if (count < 0) return DIS_FMT_FAIL;

    const char *const displaystr = StringTables[dec.mode]
                                               [snapshot->cpu.exec_cycle];
    switch (snapshot->cpu.exec_cycle) {
    case 0:
        count = sprintf(dis + count, "%s", displaystr);
        break;
    case 1:
        count = sprintf(dis + count, displaystr, snapshot->cpu.databus);
        break;
    case 2:
        count = sprintf(dis + count, displaystr,
                        bytowr(snapshot->cpu.addrlow_latch,
                               snapshot->cpu.databus));
        break;
    default:
        return DIS_INV_DISPCYC;
    }

    assert((unsigned int)count < DIS_MNEM_SIZE);
    return count;
}
