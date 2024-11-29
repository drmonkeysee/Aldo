//
//  ppu.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#ifndef Aldo_ppu_h
#define Aldo_ppu_h

#include "bus.h"
#include "bytes.h"
#include "ctrlsignal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct aldo_snapshot;

// The Ricoh RP2C02 Picture Processing Unit (PPU) is a
// fixed-function IC that generates the NES video signal.
struct aldo_rp2c02 {
    // Video Bus: video component connections
    aldo_bus *vbus;         // Non-owning Pointer

    // PPU Registers
    struct {
        bool
            nl: 1,          // Nametable Address Select Low; 1 = add $400
            nh: 1,          // Nametable Address Select High; 1 = add $800
            i: 1,           // VRAM Address Increment Mode, for NT access;
                            // 0: add 1 (next tile), 1: add 32 (next row).
            s: 1,           // Sprite Pattern Table Address;
                            // 0: $0000, 1: $1000, ignored if h = 1.
            b: 1,           // Background Pattern Table Address;
                            // 0: $0000, 1: $1000.
            h: 1,           // Sprite Size (0: 8x8, 1: 8x16)
             : 1,           // PPU Master/Slave
                            // 0: read backdrop from EXT pins,
                            // 1: output color on EXT pins;
                            // grounded on normal NES so effectively unused.
            v: 1;           // NMI Enabled
    } ctrl;                 // PPUCTRL, write-only
    struct {
        bool
            g: 1,           // Grayscale
            bm: 1,          // Background Left-Column Enabled
            sm: 1,          // Sprite Left-Column Enabled
            b: 1,           // Background Rendering
            s: 1,           // Sprite Rendering
            re: 1,          // Red Emphasis
            ge: 1,          // Green Emphasis
            be: 1;          // Blue Emphasis
    } mask;                 // PPUMASK, write-only
    struct {
        bool
            o: 1,           // (5) Sprite Overflow
            s: 1,           // (6) Sprite 0 Hit
            v: 1;           // (7) VBlank
    } status;               // PPUSTATUS, read-only
    // NOTE: OAMDATA, PPUSCROLL, PPUADDR, and PPUDATA are ports to internal
    // components like OAM, t, and x, so are not modeled as storage locations.
    uint8_t oamaddr;        // OAMADDR: OAM Data Address, write-only

    // Datapath
    enum aldo_sigstate rst; // RESET detection latch
    uint16_t vaddrbus;      // VRAM Address Bus (14 bits)
    uint8_t regsel,         // Register Selection (3 bits);
                            // wired to lowest 3 bits of CPU address bus.
            regbus,         // Register Data Bus
            vdatabus,       // VRAM Data Bus (actually shared with lower
            video;          // Video Signal (6 bits)
    struct {                // 8-bits of vaddrbus but this is not modeled
        bool                // to avoid storing an extra latch).
            ale: 1,         // Address Latch Signal (output)
            intr: 1,        // Interrupt Signal (output, inverted);
                            // wired to CPU NMI.
            rst: 1,         // Reset Signal (input, inverted)
            rw: 1,          // CPU Read/Write (input, read high)
            rd: 1,          // VRAM Read (output, inverted)
            wr: 1,          // VRAM Write (output, inverted); not signaled
                            // for palette writes.
            vout: 1;        // Video Out (output); actual pin is tied to
                            // analog video output but used here to show
                            // when PPU is outputting a visible pixel.
    } signal;

    // Pixel Render Pipeline
    struct {
        uint16_t bgs[2];    // Background Tile Shifter
        uint8_t at,         // Attribute Table Fetch
                atb,        // Attribute Latch Bit (0, 2, 4, 6)
                ats[2],     // Attribute Table Shifter
                bg[2],      // Background Tile Fetch
                mux,        // Multiplexed Pixel Selection
                nt,         // Nametable Fetch
                pal,        // Palette Index
                px;         // Pixel Color Output
        bool atl[2];        // Attribute Table Latch (2 bits)
        // TODO: 8 sprite select/shifts
    } pxpl;

    // Internal Registers and Control Flags
    int
        dot,                // Current Dot
        line;               // Current Scanline
    uint16_t
        t,                  // Temp VRAM/Scrolling Address (15 bits)
        v;                  // Current VRAM/Scrolling Address (15 bits)
    uint8_t rbuf,           // PPUDATA Read Buffer
            x;              // Fine X Scroll (3 bits)
    bool
        bflt,               // Bus fault
        cvp,                // Pending CPU VRAM Operation
        odd,                // Current frame is even or odd
        w;                  // Write latch for x2 registers

    // Internal Memory
    uint8_t oam[256],       // Object Attribute Memory: internal storage
                            // for sprite attributes.
            soam[32],       // Secondary OAM: up to 8 active sprites for
                            // current scanline.
            palette[28];    // Palette indices: 1 backdrop, 3 unused,
                            // 12 background, 12 sprite.
};

struct aldo_ppu_coord { int dot, line; };

extern const uint16_t Aldo_PaletteStartAddr;
extern const int Aldo_DotsPerFrame;

void aldo_ppu_connect(struct aldo_rp2c02 *self, aldo_bus *mbus);
void aldo_ppu_powerup(struct aldo_rp2c02 *self);
void aldo_ppu_zeroram(struct aldo_rp2c02 *self);

bool aldo_ppu_cycle(struct aldo_rp2c02 *self);

void aldo_ppu_bus_snapshot(const struct aldo_rp2c02 *self,
                           struct aldo_snapshot *snp);
void aldo_ppu_vid_snapshot(const struct aldo_rp2c02 *self,
                           struct aldo_snapshot *snp);
bool aldo_ppu_dumpram(const struct aldo_rp2c02 *self, FILE *f);
struct aldo_ppu_coord aldo_ppu_trace(const struct aldo_rp2c02 *self,
                                     int adjustment);
struct aldo_ppu_coord aldo_ppu_screendot(const struct aldo_rp2c02 *self);

#endif
