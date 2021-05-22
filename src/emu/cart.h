//
//  cart.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#ifndef Aldo_emu_cart_h
#define Aldo_emu_cart_h

#include "bus.h"
#include "snapshot.h"

#include <stdint.h>
#include <stdio.h>

// X(symbol, value, error string)
#define CART_ERRCODE_X \
X(CART_UNKNOWN_ERR, -1, "UNKNOWN CART LOAD ERROR") \
X(CART_IO_ERR, -2, "FILE READ ERROR") \
X(CART_PRG_SIZE, -3, "PROGRAM TOO LARGE") \
X(CART_EOF, -4, "UNEXPECTED EOF") \
X(CART_OBSOLETE, -5, "OBSOLETE FORMAT") \
X(CART_ADDR_UNAVAILABLE, -6, "BUS ADDRESS UNAVAILABLE")

enum {
#define X(s, v, e) s = v,
    CART_ERRCODE_X
#undef X
};

typedef struct cartridge cart;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *cart_errstr(int err);

// NOTE: if cart_create returns non-zero error code, *c is unmodified
int cart_create(cart **c, FILE *f);
void cart_free(cart *self);

uint8_t *cart_prg_bank(cart *self);

int cart_cpu_connect(cart *self, bus *b, uint16_t addr);

void cart_info_write(cart *self, FILE *f, bool verbose);
void cart_snapshot(cart *self, struct console_state *snapshot);

#endif
