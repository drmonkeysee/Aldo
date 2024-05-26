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
#include "ctrlsignal.h"
#include "snapshot.h"

// The Ricoh RP2C02 Picture Processing Unit (PPU) is a
// fixed-function IC that generates the NES video signal.
struct rp2c02 {
    // Buses
    bus *mbus,                  // Main Bus: to/from CPU; Non-owning Pointer
        *vbus;                  // Video Bus: video component connections

    // PPU Registers
    struct {
        unsigned int nn: 2;     // Base Nametable Address;
        bool                    // 0: $2000, 1: $2400, 2: $2800, 3: $2C00.
            i: 1,               // VRAM Address Increment Mode, for NT access;
                                // 0: add 1 (next tile), 1: add 32 (next row).
            s: 1,               // Sprite Pattern Table Address;
                                // 0: $0000, 1: $1000, ignored if h = 1.
            b: 1,               // Background Pattern Table Address;
            h: 1,               // Sprite Size (0: 8x8, 1: 8x16)
            p: 1,               // PPU Master/Slave
                                // 0: $0000, 1: $1000.
            v: 1;               // NMI Enabled
    } ppuctrl;                  // PPUCTRL, write-only
    struct {
        bool
            g: 1,               // Greyscale
            bm: 1,              // Background Left-Column Enabled
            sm: 1,              // Sprite Left-Column Enabled
            b: 1,               // Background Rendering
            s: 1;               // Sprite Rendering
        unsigned int bgr: 3;    // Blue/Green/Red Emphasis
    } ppumask;                  // PPUMASK, write-only
    struct {
        bool
            o: 1,               // (5) Sprite Overflow
            s: 1,               // (6) Sprite 0 Hit
            v: 1;               // (7) VBlank
    } ppustatus;                // PPUSTATUS, read-only
    uint8_t oamaddr,            // OAM Read/Write Address, write-only
            oamdata,            // OAM Data, read/write
            ppuscroll,          // Fine Scroll Position, write-only;
                                // 1st write = X scroll, 2nd write = Y scroll.
            ppuaddr,            // VRAM Address, write-only;
                                // 1st write = MSB, 2nd write = LSB
                                // (note this is opposite of little-endian!)
            ppudata,            // VRAM Data, read/write;
                                // must be read twice to prime read-buffer.
            oamdma;             // OAM DMA High Address, write-only

    // Datapath
    enum csig_state res;        // RESET detection latch
    int dot,                    // Current Dot
        line;                   // Current Scanline

    uint16_t vaddrbus;          // VRAM Address Bus (14 bits)
    uint8_t regbus,             // Register Data Bus
            vdatabus;           // VRAM Data Bus (actually shared with lower
                                // 8-bits of vaddrbus but this is not modeled
                                // to avoid storing an extra latch).
    struct {
        unsigned int reg: 3;    // Register Select Signal;
                                // wired to lowest 3 bits of CPU address bus
        bool
            intr: 1,            // Interrupt Signal (output, inverted);
                                // wired to CPU NMI.
            res: 1;             // Reset Signal (input, inverted)
    } signal;

    // Internal Registers
    uint16_t
        t,                      // Temp VRAM Address (15 bits)
        v;                      // Current VRAM Address (15 bits)
    uint8_t rbuf,               // PPUDATA Read Buffer
            x;                  // Fine X Scroll (3 bits)
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
