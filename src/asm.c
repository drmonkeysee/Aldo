//
//  asm.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#include "asm.h"

#include "emu/bytes.h"
#include "emu/decode.h"
#include "emu/traits.h"

#include <assert.h>
#include <stdio.h>

static const char *restrict const Mnemonics[] = {
#define X(s, ...) #s,
    DEC_INST_X
#undef X
};

static const int InstLens[] = {
#define X(s, b, ...) b,
    DEC_ADDRMODE_X
#undef X
};

#define STRTABLE(s) s##_StringTable

#define X(s, b, ...) static const char *restrict const STRTABLE(s)[] = { \
    __VA_ARGS__ \
};
    DEC_ADDRMODE_X
#undef X

static const char *restrict const *const StringTables[] = {
#define X(s, b, ...) STRTABLE(s),
    DEC_ADDRMODE_X
#undef X
};

static int print_raw(uint16_t addr, const uint8_t *dispc, int instlen,
                     char dis[restrict static DIS_INST_SIZE])
{
    int total, count;
    total = count = sprintf(dis, "$%04X: ", addr);
    if (count < 0) return ASM_FMT_FAIL;

    for (int i = 0; i < instlen; ++i) {
        count = sprintf(dis + total, "%02X ", *(dispc + i));
        if (count < 0) return ASM_FMT_FAIL;
        total += count;
    }

    return total;
}

static int print_mnemonic(const struct decoded *dec, const uint8_t *dispc,
                          int instlen, char dis[restrict])
{
    int total, count;
    total = count = sprintf(dis, "%s ", Mnemonics[dec->instruction]);
    if (count < 0) return ASM_FMT_FAIL;

    const char *restrict const *const strtable = StringTables[dec->mode];
    switch (instlen) {
    case 1:
        // NOTE: nothing else to print, trim the trailing space
        dis[--count] = '\0';
        break;
    case 2:
        count = sprintf(dis + total, strtable[instlen - 1], *(dispc + 1));
        break;
    case 3:
        count = sprintf(dis + total, strtable[instlen - 1], batowr(dispc + 1));
        break;
    default:
        return ASM_INV_ADDRMD;
    }
    if (count < 0) return ASM_FMT_FAIL;
    return total + count;
}

//
// Public Interface
//

const char *dis_errstr(int error)
{
    switch (error) {
#define X(s, v, e) case s: return e;
        ASM_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

int dis_inst(uint16_t addr, const uint8_t *dispc, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE])
{
    assert(dispc != NULL);
    assert(bytesleft >= 0);
    assert(dis != NULL);

    if (bytesleft == 0) return 0;

    const struct decoded dec = Decode[*dispc];
    const int instlen = InstLens[dec.mode];
    if (bytesleft < instlen) return ASM_EOF;

    int count;
    unsigned int total;
    total = count = print_raw(addr, dispc, instlen, dis);
    if (count < 0) return count;

    // NOTE: padding between raw bytes and disassembled instruction
    count = sprintf(dis + total, "%*s", (4 - instlen) * 3, "");
    if (count < 0) return ASM_FMT_FAIL;
    total += count;

    count = print_mnemonic(&dec, dispc, instlen, dis + total);
    if (count < 0) return count;
    total += count;

    assert(total < DIS_INST_SIZE);
    return instlen;
}

int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE])
{
    assert(snapshot != NULL);
    assert(dis != NULL);

    const uint16_t instaddr = snapshot->cpu.current_instruction;
    if (instaddr < CpuCartMinAddr) return ASM_EOF;

    const struct decoded dec = Decode[snapshot->cpu.opcode];
    const int instlen = InstLens[dec.mode];
    // NOTE: detect pc overflow
    if ((uint16_t)(instaddr + instlen) < CpuCartMinAddr) return ASM_EOF;

    int count;
    unsigned int total;
    total = count = sprintf(dis, "%s ", Mnemonics[dec.instruction]);
    if (count < 0) return ASM_FMT_FAIL;

    const int max_offset = 1 + (instlen / 3),
              displayidx = snapshot->cpu.exec_cycle < max_offset
                           ? snapshot->cpu.exec_cycle
                           : max_offset;
    const char *const displaystr = StringTables[dec.mode][displayidx];
    const uint16_t rom_idx = instaddr & CpuCartAddrMask;
    switch (displayidx) {
    case 0:
        count = sprintf(dis + total, "%s", displaystr);
        break;
    case 1:
        count = sprintf(dis + total, displaystr, snapshot->rom[rom_idx + 1]);
        break;
    default:
        count = sprintf(dis + total, displaystr,
                        batowr(snapshot->rom + rom_idx + 1));
        break;
    }
    if (count < 0) return ASM_FMT_FAIL;
    total += count;

    assert(total < DIS_DATAP_SIZE);
    return total;
}
