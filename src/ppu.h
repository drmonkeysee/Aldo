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
#include "snapshot.h"

// The Ricoh RP2C02 Picture Processing Unit (PPU) is a
// fixed-function IC that generates the NES video signal.
struct rp2c02 {
    // Buses
    bus *mbus,                  // Main Bus: to/from CPU; Non-owning Pointer
        *vbus;                  // Video Bus: video component connections

    int dot,                    // Current Dot
        line;                   // Current Scanline

    // Datapath
    uint8_t regd;               // Register Data Latch

    // Internal Registers
    bool
        odd,                    // Current frame is even or odd
        w;                      // Write latch for x2 registers

    // Internal Memory
    uint8_t oam[MEMBLOCK_256B], // Object Attribute Memory: internal storage
                                // for sprite attributes.
            palette[25];        // Palette indices: 1 universal background,
                                // 12 background, 12 sprite.
};

struct ppu_coord { int dot, line; };

void ppu_connect(struct rp2c02 *self, void *restrict vram, bus *mbus);
void ppu_disconnect(struct rp2c02 *self);

void ppu_powerup(struct rp2c02 *self);

int ppu_cycle(struct rp2c02 *self, int cpu_cycles);

void ppu_snapshot(const struct rp2c02 *self, struct console_state *snapshot);
struct ppu_coord ppu_pixel_trace(const struct rp2c02 *self, int adjustment);

#endif
