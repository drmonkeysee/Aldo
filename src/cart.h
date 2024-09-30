//
//  cart.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#ifndef Aldo_cart_h
#define Aldo_cart_h

#include "bus.h"
#include "snapshot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// X(symbol, name)
#define CART_FORMAT_X \
X(CRTF_RAW, "Raw ROM Image?") \
X(CRTF_ALDO, "Aldo") \
X(CRTF_INES, "iNES") \
X(CRTF_NES20, "NES 2.0") \
X(CRTF_NSF, "NES Sound Format")

enum cartformat {
#define X(s, n) s,
    CART_FORMAT_X
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
    uint8_t chr_blocks,         // CHR ROM block count; 0 indicates CHR RAM
                                //      1 block = 8KB
            mapper_id,          // Mapper ID
            prg_blocks,         // PRG double-block count
                                //      1 block = 16KB
            wram_blocks;        // WRAM block count; may be set by mapper
                                //      1 block = 8KB
    bool
        bus_conflicts,          // Cart has bus conflicts
        mapper_controlled,      // Mapper-controlled Nametable Mirroring
        mapper_implemented,     // Mapper is implemented
        trainer,                // Trainer data present
        wram;                   // PRG RAM banks present
};

struct cartinfo {
    enum cartformat format;
    union {
        struct ines_header ines_hdr;
    };
};

// X(symbol, value, error string)
#define CART_ERRCODE_X \
X(CART_ERR_UNKNOWN, -1, "UNKNOWN CART LOAD ERROR") \
X(CART_ERR_IO, -2, "FILE READ ERROR") \
X(CART_ERR_IMG_SIZE, -3, "ROM IMAGE TOO LARGE") \
X(CART_ERR_EOF, -4, "UNEXPECTED EOF") \
X(CART_ERR_OBSOLETE, -5, "OBSOLETE FORMAT") \
X(CART_ERR_FORMAT, -6, "FORMAT UNSUPPORTED") \
X(CART_ERR_FMT, -7, "FORMATTED OUTPUT FAILURE") \
X(CART_ERR_NOCART, -8, "NO CART")

enum {
#define X(s, v, e) s = v,
    CART_ERRCODE_X
#undef X
};

enum {
    CART_FMT_SIZE = 17, // Format extended name is at most 16 chars
};

struct blockview {
    size_t ord, size;
    const uint8_t *mem; // Non-owning Pointer
};

typedef struct cartridge cart;

#include "bridgeopen.h"
//
// MARK: - Export
//

br_libexport
const char *cart_errstr(int err) br_nothrow;

// NOTE: if returns non-zero error code, *c is unmodified
br_libexport br_checkerror
int cart_create(cart **c, FILE *f) br_nothrow;
br_libexport
void cart_free(cart *self) br_nothrow;

br_libexport
const char *cart_formatname(enum cartformat format) br_nothrow;
br_libexport
const char *cart_mirrorname(enum nt_mirroring mirror) br_nothrow;
br_libexport br_checkerror
int cart_format_extname(cart *self,
                        char buf[br_noalias_csz(CART_FMT_SIZE)]) br_nothrow;
br_libexport
void cart_write_info(cart *self, const char *br_noalias name, bool verbose,
                     FILE *f) br_nothrow;
br_libexport
void cart_getinfo(cart *self, struct cartinfo *info) br_nothrow;

br_libexport
struct blockview cart_prgblock(cart *self, size_t i) br_nothrow;
br_libexport
struct blockview cart_chrblock(cart *self, size_t i) br_nothrow;

//
// MARK: - Internal
//

bool cart_mbus_connect(cart *self, bus *b) br_nothrow;
void cart_mbus_disconnect(cart *self, bus *b) br_nothrow;
bool cart_vbus_connect(cart *self, bus *b) br_nothrow;
void cart_vbus_disconnect(cart *self, bus *b) br_nothrow;

void cart_write_dis_header(cart *self, const char *br_noalias name,
                           FILE *f) br_nothrow;
void cart_snapshot(cart *self, struct aldo_snapshot *snp) br_nothrow;
#include "bridgeclose.h"

#endif
