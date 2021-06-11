//
//  dis.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#ifndef Aldo_dis_h
#define Aldo_dis_h

#include "control.h"
#include "emu/cart.h"
#include "emu/snapshot.h"

#include <stddef.h>
#include <stdint.h>

#define DIS_INST_SIZE 31u   // Disassembled instruction is at most 30 chars
#define DIS_DATAP_SIZE 12u  // Disassembled datapath is at most 11 chars

// X(symbol, value, error string)
#define DIS_ERRCODE_X \
X(DIS_ERR_FMT_FAIL, -1, "OUTPUT FAIL") \
X(DIS_ERR_EOF, -2, "UNEXPECTED EOF") \
X(DIS_ERR_INV_ADDRMD, -3, "INVALID ADDRMODE") \
X(DIS_ERR_CHRROM, -4, "NO CHR ROM FOUND")

enum {
#define X(s, v, e) s = v,
    DIS_ERRCODE_X
#undef X
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *dis_errstr(int err);

// NOTE: when dis_ functions return 0 the input buffer is untouched
int dis_inst(uint16_t addr, const uint8_t *restrict bytes, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE]);
int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE]);
int dis_cart(cart *cart, const struct control *appstate);
int dis_cart_chr(cart *cart);

#endif
