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
#include <stdio.h>
#include <stdlib.h>

// X(symbol, description)
#define CART_FORMAT_X \
X(UNKNOWN, "Unknown") \
X(ALDOBIN, "Aldo Binary") \
X(INES, "iNES") \
X(NES20, "NES 2.0")

#define CRTF_ENUM(s) CRTF_##s

enum cartformat {
#define X(s, d) CRTF_ENUM(s),
    CART_FORMAT_X
#undef X
};

// A NES game cartridge including PRG and CHR ROM banks,
// WRAM or save-state RAM, memory mapper circuitry, and metadata headers.

struct cartridge {
    enum cartformat format;
    uint8_t prg[ROM_SIZE];  // TODO: assume a single bank of 32KB PRG for now
};

static const char *format_description(enum cartformat f)
{
    switch (f) {
#define X(s, d) case CRTF_ENUM(s): return d;
        CART_FORMAT_X
#undef X
    default:
        assert(((void)"BAD CART FORMAT", false));
    }
}

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

int cart_create(cart **c, FILE *f)
{
    assert(c != NULL);
    assert(f != NULL);

    struct cartridge *const self = malloc(sizeof *self);
    const size_t bufsize = sizeof self->prg / sizeof self->prg[0],
                 count = fread(self->prg, sizeof self->prg[0], bufsize, f);
    int error;
    if (feof(f)) {
        *c = self;
        return 0;
    } else if (ferror(f)) {
        error = CART_IO_ERR;
    } else if (count == bufsize) {
        error = CART_PRG_SIZE;
    } else {
        error = CART_UNKNOWN;
    }
    cart_free(self);
    return error;
}

void cart_free(cart *self)
{
    free(self);
}

uint8_t *cart_prg_bank(cart *self)
{
    assert(self != NULL);

    return self->prg;
}

void cart_info_write(cart *self, FILE *f)
{
    fprintf(f, "Format: %s\n", format_description(self->format));
}

void cart_snapshot(cart *self, struct console_state *snapshot)
{
    assert(self != NULL);

    snapshot->rom = cart_prg_bank(self);
}
