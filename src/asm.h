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
X(ASM_FMT_FAIL, -1, "OUTPUT FAIL") \
X(ASM_EOF, -2, "UNEXPECTED EOF") \
X(ASM_INV_ADDRMD, -3, "INVALID ADDRMODE") \
X(ASM_RANGE, -4, "OUT OF RANGE")

enum {
#define X(s, v, e) s = v,
    ASM_ERRCODE_X
#undef X
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *dis_errstr(int error);

// NOTE: when dis_ functions return 0 the input buffer is untouched
int dis_inst(uint16_t addr, const uint8_t *restrict bytes, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE]);
int dis_mem(uint16_t addr, const struct console_state *snapshot,
            char dis[restrict static DIS_INST_SIZE]);
int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE]);

#endif
