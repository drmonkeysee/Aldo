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
#define ALDO_CART_FORMAT_X \
X(CRTF_RAW, "Raw ROM Image?") \
X(CRTF_ALDO, "Aldo") \
X(CRTF_INES, "iNES") \
X(CRTF_NES20, "NES 2.0") \
X(CRTF_NSF, "NES Sound Format")

enum aldo_cartformat {
#define X(s, n) ALDO_##s,
    ALDO_CART_FORMAT_X
#undef X
};

// X(symbol, name)
#define ALDO_CART_NTMIRROR_X \
X(NTM_HORIZONTAL, "Horizontal") \
X(NTM_VERTICAL, "Vertical") \
X(NTM_1SCREEN, "Single-Screen") \
X(NTM_4SCREEN, "4-Screen VRAM") \
X(NTM_OTHER, "Mapper-Specific")

enum aldo_nt_mirroring {
#define X(s, n) ALDO_##s,
    ALDO_CART_NTMIRROR_X
#undef X
};

// iNES File Header
// TODO: ignoring following fields for now:
//  - VS/Playchoice system indicator (does anyone care?)
//  - TV System (PAL ROMs don't seem to set the flags so again who cares)
//  - redundant indicators in byte 10
struct aldo_ines_header {
    enum aldo_nt_mirroring mirror;  // Nametable Mirroring
    uint8_t chr_blocks,             // CHR ROM block count; 0 indicates CHR RAM
                                    //      1 block = 8KB
            mapper_id,              // Mapper ID
            prg_blocks,             // PRG double-block count
                                    //      1 block = 16KB
            wram_blocks;            // WRAM block count; may be set by mapper
                                    //      1 block = 8KB
    bool
        bus_conflicts,              // Cart has bus conflicts
        mapper_controlled,          // Mapper-controlled Nametable Mirroring
        mapper_implemented,         // Mapper is implemented
        trainer,                    // Trainer data present
        wram;                       // PRG RAM banks present
};

struct aldo_cartinfo {
    enum aldo_cartformat format;
    union {
        struct aldo_ines_header ines_hdr;
    };
};

// X(symbol, value, error string)
#define ALDO_CART_ERRCODE_X \
X(CART_ERR_UNKNOWN, -1, "UNKNOWN CART LOAD ERROR") \
X(CART_ERR_IO, -2, "FILE I/O ERROR") \
X(CART_ERR_IMG_SIZE, -3, "ROM IMAGE TOO LARGE") \
X(CART_ERR_EOF, -4, "UNEXPECTED EOF") \
X(CART_ERR_OBSOLETE, -5, "OBSOLETE FORMAT") \
X(CART_ERR_FORMAT, -6, "FORMAT UNSUPPORTED") \
X(CART_ERR_FMT, -7, "FORMATTED OUTPUT FAILURE") \
X(CART_ERR_NOCART, -8, "NO CART") \
X(CART_ERR_ERNO, -9, "SYSTEM ERROR")

enum {
#define X(s, v, e) ALDO_##s = v,
    ALDO_CART_ERRCODE_X
#undef X
};

enum {
    ALDO_CART_FMT_SIZE = 17,
};

struct aldo_blockview {
    size_t ord, size;
    const uint8_t *mem; // Non-owning Pointer
};

typedef struct aldo_cartridge aldo_cart;

#include "bridgeopen.h"
//
// MARK: - Export
//

br_libexport
const char *aldo_cart_errstr(int err) br_nothrow;

// NOTE: if returns non-zero error code, *c is unmodified
br_libexport br_checkerror
int aldo_cart_create(aldo_cart **c, FILE *f) br_nothrow;
br_libexport
void aldo_cart_free(aldo_cart *self) br_nothrow;

br_libexport
const char *aldo_cart_formatname(enum aldo_cartformat format) br_nothrow;
br_libexport
const char *aldo_cart_mirrorname(enum aldo_nt_mirroring mirror) br_nothrow;
br_libexport br_checkerror
int
aldo_cart_format_extname(aldo_cart *self,
                         char buf[br_nacsz(ALDO_CART_FMT_SIZE)]) br_nothrow;
br_libexport
int aldo_cart_write_info(aldo_cart *self, const char *br_noalias name,
                         bool verbose, FILE *f) br_nothrow;
br_libexport
void aldo_cart_getinfo(aldo_cart *self, struct aldo_cartinfo *info) br_nothrow;

br_libexport
struct aldo_blockview aldo_cart_prgblock(aldo_cart *self, size_t i) br_nothrow;
br_libexport
struct aldo_blockview aldo_cart_chrblock(aldo_cart *self, size_t i) br_nothrow;

//
// MARK: - Internal
//

bool aldo_cart_mbus_connect(aldo_cart *self, aldo_bus *b) br_nothrow;
void aldo_cart_mbus_disconnect(aldo_cart *self, aldo_bus *b) br_nothrow;
bool aldo_cart_vbus_connect(aldo_cart *self, aldo_bus *b) br_nothrow;
void aldo_cart_vbus_disconnect(aldo_cart *self, aldo_bus *b) br_nothrow;

int aldo_cart_write_dis_header(aldo_cart *self, const char *br_noalias name,
                               FILE *f) br_nothrow;
void aldo_cart_snapshot(aldo_cart *self, struct aldo_snapshot *snp) br_nothrow;
#include "bridgeclose.h"

#endif
