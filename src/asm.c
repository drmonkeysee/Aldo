//
//  asm.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#include "asm.h"

#include <assert.h>
#include <stdio.h>

// NOTE: undefined behavior if mnemonic requires more bytes than are pointed
// to by dispc.
// TODO: add parameter to disassemble based on current cycle vs fully decoded
int dis_mnemonic(const uint8_t *dispc, char dis[restrict static DIS_MNEM_SIZE])
{
    assert(dispc != NULL);
    assert(dis != NULL);

    const uint8_t opcode = *dispc;
    int count;
    if (opcode == 0xea) {
        count = sprintf(dis, "NOP");
        if (count < 0) return DIS_FMT_FAIL;
    } else {
        // TODO: pretend unk operand is
        // indirect absolute address (longest operand in chars)
        uint16_t operand = *(dispc + 1) | (*(dispc + 2) << 8);
        count = sprintf(dis, "UNK ($%04X)", operand);
        if (count < 0) return DIS_FMT_FAIL;
    }
    return count;
}

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
    // TODO: get rid of this redundant instruction check with mnemonic
    if (instruction == 0xea) {
        total = count = sprintf(dis, "$%04X: %02X          ", addr,
                                instruction);
        if (count < 0) return DIS_FMT_FAIL;

        count = dis_mnemonic(dispc, dis + count);
        if (count < 0) return count;

        total += count;
        assert(total < DIS_INST_SIZE);
        return 1;
    } else {
        if (bytesleft < 3) return DIS_EOF;
        total = count = sprintf(dis, "$%04X: %02X %02X %02X    ", addr,
                                instruction, *(dispc + 1), *(dispc + 2));
        if (count < 0) return DIS_FMT_FAIL;

        count = dis_mnemonic(dispc, dis + count);
        if (count < 0) return count;

        total += count;
        assert(total < DIS_INST_SIZE);
        // TODO: pretend UNK instruction takes 3 bytes to test UI layout
        return 3;
    }
}

const char *dis_errstr(int error)
{
    switch (error) {
    case DIS_FMT_FAIL:
        return "OUTPUT FAIL";
    case DIS_EOF:
        return "UNEXPECTED EOF";
    default:
        return "UNKNOWN ERR";
    }
}
