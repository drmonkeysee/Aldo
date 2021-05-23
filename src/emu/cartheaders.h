//
//  cartheaders.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#ifndef Aldo_emu_cartheaders_h
#define Aldo_emu_cartheaders_h

#include <stdbool.h>
#include <stdint.h>

enum nt_mirroring {
    NTM_HORIZONTAL,
    NTM_VERTICAL,
    NTM_1SCREEN,
    NTM_4SCREEN,
    NTM_OTHER,
};

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

#endif
