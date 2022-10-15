//
//  cartinfo.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/12/22.
//

#ifndef Aldo_cartinfo_h
#define Aldo_cartinfo_h

#include <stdbool.h>
#include <stdint.h>

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
    const char *filepath;       // Non-owning Pointer
    enum cartformat format;
    union {
        struct ines_header ines_hdr;
    };
};

#endif
