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
X(CART_ERR_UNKNOWN, -1, "UNKNOWN CART LOAD ERROR") \
X(CART_ERR_IO, -2, "FILE READ ERROR") \
X(CART_ERR_IMG_SIZE, -3, "ROM IMAGE TOO LARGE") \
X(CART_ERR_EOF, -4, "UNEXPECTED EOF") \
X(CART_ERR_OBSOLETE, -5, "OBSOLETE FORMAT") \
X(CART_ERR_ADDR_UNAVAILABLE, -6, "BUS ADDRESS UNAVAILABLE") \
X(CART_ERR_FORMAT, -7, "FORMAT UNSUPPORTED")

enum {
#define X(s, v, e) s = v,
    CART_ERRCODE_X
#undef X
};

// X(symbol, name)
#define CART_INES_NTMIRROR_X \
X(NTM_HORIZONTAL, "Horizontal") \
X(NTM_VERTICAL, "Vertical") \
X(NTM_1SCREEN, "Single-Screen") \
X(NTM_4SCREEN, "4-Screen VRAM") \
X(NTM_OTHER, "Mapper-Specific")

enum nt_mirroring {
#define X(s, n) s,
    CART_INES_NTMIRROR_X
#undef X
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
         mapper_implemented,    // Mapper is implemented
         wram,                  // PRG RAM banks present
         trainer;               // Trainer data present
};

struct bankview {
    size_t bank;
    const uint8_t *mem;
    uint16_t size;
};

typedef struct cartridge cart;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *cart_errstr(int err);

// NOTE: if cart_create returns non-zero error code, *c is unmodified
int cart_create(cart **c, FILE *f);
void cart_free(cart *self);

struct bankview cart_prgbank(cart *self, size_t i);

int cart_cpu_connect(cart *self, bus *b, uint16_t addr);
void cart_cpu_disconnect(cart *self, bus *b, uint16_t addr);

void cart_write_info(cart *self, FILE *f, bool verbose);
void cart_write_dis_header(cart *self, FILE *f);
void cart_snapshot(cart *self, struct console_state *snapshot);

#endif
