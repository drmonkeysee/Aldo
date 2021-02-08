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
#define X(i) #i,
    DEC_INST_X
#undef X
};

static const char *restrict const DiscardedData = "\u2205";

// TODO: get rid of this redundant logic between the two dis functions

int dis_inst(uint16_t addr, const uint8_t *dispc, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE])
{
    assert(dispc != NULL);
    assert(bytesleft >= 0);
    assert(dis != NULL);

    if (bytesleft == 0) return 0;

    const uint8_t instruction = *dispc;
    int count;
    unsigned int total;
    if (instruction == 0xea) {
        total = count = sprintf(dis, "$%04X: %02X          ", addr,
                                instruction);
        if (count < 0) return DIS_FMT_FAIL;

        count = sprintf(dis + count, "NOP");
        if (count < 0) return DIS_FMT_FAIL;

        total += count;
        assert(total < DIS_INST_SIZE);
        return 1;
    } else {
        if (bytesleft < 3) return DIS_EOF;
        total = count = sprintf(dis, "$%04X: %02X %02X %02X    ", addr,
                                instruction, *(dispc + 1), *(dispc + 2));
        if (count < 0) return DIS_FMT_FAIL;

        // TODO: pretend unk operand is
        // indirect absolute address (longest operand in chars)
        count = sprintf(dis + count, "UNK ($%04X)", batowr(dispc + 1));
        if (count < 0) return DIS_FMT_FAIL;

        total += count;
        assert(total < DIS_INST_SIZE);
        // TODO: pretend UNK instruction takes 3 bytes to test UI layout
        return 3;
    }
}

const char *dis_errstr(int error)
{
    switch (error) {
#define X(i, v, s) case DIS_ ## i: return s;
    DIS_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

// NOTE: undefined behavior if mnemonic requires more bytes than are pointed
// to by dispc.
// TODO: add parameter to disassemble based on current cycle vs fully decoded
int dis_datapath(uint8_t opcode, uint16_t operand, uint8_t cycle,
                 char dis[restrict static DIS_MNEM_SIZE])
{
    assert(dis != NULL);

    int count;
    if (opcode == 0xea) {
        count = sprintf(dis, "NOP");
        if (count < 0) return DIS_FMT_FAIL;
        if (cycle == 1) {
            count = sprintf(dis + count, " %s", DiscardedData);
            if (count < 0) return DIS_FMT_FAIL;
        }
    } else {
        // TODO: pretend unk operand is
        // indirect absolute address (longest operand in chars)
        count = sprintf(dis, "UNK ($%04X)", operand);
        if (count < 0) return DIS_FMT_FAIL;
    }
    assert((unsigned int)count < DIS_MNEM_SIZE);
    return count;
}
