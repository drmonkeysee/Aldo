//
//  asm.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#ifndef Aldo_asm_h
#define Aldo_asm_h

#include "emu/snapshot.h"

#include <stddef.h>
#include <stdint.h>

#define DIS_INST_SIZE 31u   // Disassembled instruction is at most 30 chars
#define DIS_DATAP_SIZE 12u  // Disassembled datapath is at most 11 chars

// X(symbol, value, error string)
#define DIS_ERRCODE_X \
X(FMT_FAIL, -1, "OUTPUT FAIL") \
X(EOF, -2, "UNEXPECTED EOF") \
X(INV_ADDRM, -3, "INVALID ADDRMODE") \
X(INV_DISPCYC, -4, "INVALID DISPLAY CYCLE")

enum {
#define X(s, v, e) DIS_##s = v,
    DIS_ERRCODE_X
#undef X
};

int dis_inst(uint16_t addr, const uint8_t *dispc, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE]);

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *dis_errstr(int error);

int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE]);

#endif
