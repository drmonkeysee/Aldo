//
//  cart.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#include "cart.h"

#include "traits.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// A NES game cartridge including PRG and CHR ROM banks,
// WRAM or save-state RAM, memory mapper circuitry, and metadata headers.

struct cartridge {
    uint8_t prg[ROM_SIZE];  // TODO: assume a single bank of 32KB PRG for now
};

//
// Public Interface
//

const char *cart_errstr(int error)
{
    switch (error) {
#define X(s, v, e) case s: return e;
        CART_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

int cart_create(cart *c, FILE *f)
{
    assert(c != NULL);
    assert(f != NULL);

    struct cartridge *const self = malloc(sizeof *self);
    const size_t bufsize = sizeof self->prg / sizeof self->prg[0],
                 count = fread(self->prg, sizeof self->prg[0], bufsize, f);
    if (feof(f)) {
        c = self;
        return 0;
    } else if (ferror(f)) {
        return CART_IO_ERR;
    } else if (count == bufsize) {
        return CART_PRG_SIZE;
    }
    return CART_UNKNOWN;
}

void cart_free(cart *self)
{
    free(self);
}
