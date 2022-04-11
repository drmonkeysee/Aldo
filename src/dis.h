//
//  dis.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#ifndef Aldo_dis_h
#define Aldo_dis_h

#include "cart.h"
#include "control.h"
#include "cpu.h"
#include "snapshot.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// X(symbol, value, error string)
#define DIS_ERRCODE_X \
X(DIS_ERR_FMT, -1, "FORMATTED OUTPUT FAILURE") \
X(DIS_ERR_EOF, -2, "UNEXPECTED EOF") \
X(DIS_ERR_INV_ADDRMD, -3, "INVALID ADDRMODE") \
X(DIS_ERR_CHRROM, -4, "NO CHR ROM FOUND") \
X(DIS_ERR_CHRSZ, -5, "INVALID CHR ROM SIZE") \
X(DIS_ERR_ERNO, -6, "SYSTEM ERROR")

enum {
#define X(s, v, e) s = v,
    DIS_ERRCODE_X
#undef X
};

enum {
    DIS_DATAP_SIZE = 12,    // Disassembled datapath is at most 11 chars
    DIS_INST_SIZE = 28,     // Disassembled instruction is at most 27 chars
    DIS_PEEK_SIZE = 20,     // Peek expression is at most 19 chars
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *dis_errstr(int err);

// NOTE: when dis_ functions return 0 the input buffer is untouched
int dis_inst(uint16_t addr, const uint8_t *restrict bytes, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE]);
int dis_peek(uint16_t addr, struct mos6502 *cpu,
             const struct console_state *snapshot,
             char dis[restrict static DIS_PEEK_SIZE]);
int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE]);
int dis_cart_prg(cart *cart, const struct control *appstate, FILE *f);
int dis_cart_chrbank(const struct bankview *bv, int scale, FILE *f);
int dis_cart_chr(cart *cart, const struct control *appstate, FILE *f);

#endif
