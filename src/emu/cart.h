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

#include <stdbool.h>
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

enum nt_mirroring {
    NTM_HORIZONTAL,
    NTM_VERTICAL,
    NTM_1SCREEN,
    NTM_4SCREEN,
    NTM_OTHER,
};

// iNES File Header
// TODO: ignoring following fields for now:
//  - VS/Playchoice system indicator (does anyone care?)
//  - TV System (PAL ROMs don't seem to set the flags so again who cares)
//  - redundant indicators in byte 10
struct ines_header {
    enum nt_mirroring mirror;   // Nametable Mirroring
    uint8_t chr_chunks,         // CHR ROM chunk count; 0 indicates CHR RAM
            mapper_id,          // Mapper ID
            wram_chunks,        // WRAM chunk count; may be set by mapper
            prg_chunks;         // PRG double-chunk count
    bool bus_conflicts,         // Cart has bus conflicts
         mapper_controlled,     // Mapper-controlled Nametable Mirroring
         wram,                  // PRG RAM banks present
         trainer;               // Trainer data present
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
