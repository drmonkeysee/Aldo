//
//  ppu.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#ifndef Aldo_ppu_h
#define Aldo_ppu_h

#include <stdbool.h>
#include <stdint.h>

#include "bus.h"
#include "bytes.h"

// The Ricoh RP2C02 Picture Processing Unit (PPU) is a
// fixed-function IC that generates the NES video signal.
struct rp2c02 {
    // Buses
    bus *mbus,                  // Main Bus: connected to the CPU;
                                // Non-owning Pointer.
        *vbus;                  // Video Bus: video component connections;
                                // Non-owning Pointer.

    // Internal Registers
    bool odd;                   // Current frame is even or odd,
                                // measured from last reset.

    // Internal Memory
    uint8_t oam[MEMBLOCK_256B], // Object Attribute Memory: internal storage
                                // for sprite attributes.
            palette[25];        // Palette indices: 1 universal background,
                                // 12 background, 12 sprite.
};

#include "bridgeopen.h"
void ppu_powerup(struct rp2c02 *self) br_nothrow;
#include "bridgeclose.h"

#endif
