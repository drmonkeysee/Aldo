//
//  cart.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#ifndef Aldo_emu_cart_h
#define Aldo_emu_cart_h

#include <stdio.h>

// X(symbol, value, error string)
#define CART_ERRCODE_X \
X(CART_IO_ERR, -1, "FILE READ ERROR") \
X(CART_PRG_SIZE, -2, "PROGRAM TOO LARGE") \
X(CART_UNKNOWN, -3, "UNKNOWN CART LOAD ERROR")

enum {
#define X(s, v, e) s = v,
    CART_ERRCODE_X
#undef X
};

typedef struct cartridge cart;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *cart_errstr(int error);

// NOTE: if cart_create returns non-zero error code, c remains untouched
int cart_create(cart *c, FILE *f);
void cart_free(cart *self);

#endif
