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
#define ASM_ERRCODE_X \
X(DIS_FMT_FAIL, -1, "OUTPUT FAIL") \
X(DIS_EOF, -2, "UNEXPECTED EOF") \
X(DIS_INV_ADDRMD, -3, "INVALID ADDRMODE") \
X(DIS_INV_DSPCYC, -4, "INVALID DISPLAY CYCLE")

enum {
#define X(s, v, e) s = v,
    ASM_ERRCODE_X
#undef X
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *dis_errstr(int error);

int dis_inst(uint16_t addr, const uint8_t *dispc, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE]);
int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE]);

#endif
