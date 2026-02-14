//
//  snapshot.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/16/21.
//

#ifndef Aldo_snapshot_h
#define Aldo_snapshot_h

#include "ctrlsignal.h"

#include <stddef.h>
#include <stdint.h>

#include "bridgeopen.h"
aldo_const size_t AldoPalSize = 4;
// CHR bit-planes are 8 bits wide and 8 bytes tall; a CHR tile is a
// 16-byte array of 2-bit palette-indexed pixels composed of two bit-planes
// where the first plane specifies the pixel low bit and the second plane
// specifies the pixel high bit.
aldo_const int AldoChrTileDim = 8;
aldo_const int AldoChrTileStride = 2 * AldoChrTileDim;
aldo_const int AldoPtTileCount = 256;
aldo_const int AldoNtCount = 2;
aldo_const int AldoNtWidth = 32;
aldo_const int AldoNtHeight = 30;
aldo_const int AldoNtTileCount = AldoNtWidth * AldoNtHeight;
aldo_const int AldoNtAttrCount = 64;
aldo_const int AldoSpriteCount = AldoNtAttrCount;

struct aldo_snapshot {
    struct {
        uint16_t program_counter;
        uint8_t accumulator, stack_pointer, status, xindex, yindex;
        struct {
            bool irq, nmi, readwrite, ready, reset, sync;
        } lines;
        struct {
            int exec_cycle;
            enum aldo_sigstate irq, nmi, rst;
            uint16_t addressbus, current_instruction;
            uint8_t addrlow_latch, addrhigh_latch, addrcarry_latch, databus,
                    opcode;
            bool busfault, instdone, jammed;
        } datapath;
    } cpu;

    struct {
        uint8_t ctrl, mask, oamaddr, status;
        struct {
            bool
                address_enable, cpu_readwrite, interrupt, read, reset,
                video_out, write;
        } lines;
        struct {
            int dot, line;
            enum aldo_sigstate rst;
            uint16_t addressbus, scrolladdr, tempaddr;
            uint8_t databus, pixel, readbuffer, register_databus,
                    register_select, xfine;
            bool busfault, cv_pending, oddframe, writelatch;
        } pipeline;
    } ppu;

    struct {
        const uint8_t           // Non-owning Pointers
            *ram, *vram,
            *oam, *secondary_oam,
            *palette;
    } mem;

    // Large fields below this point are managed via setup/cleanup to
    // avoid ending up on the stack.
    struct {
        uint8_t vectors[6];
        struct aldo_snpprg {
            size_t length;      // Number of bytes copied to pc
            uint8_t pc[96];     // 32 lines @ max 3-byte instructions
        } *curr;
    } prg;

    struct {
        uint8_t *screen;        // Non-owning Pointer
        struct {
            enum aldo_ntmirror mirror;
            struct {
                uint8_t attributes[AldoNtAttrCount],
                        tiles[AldoNtTileCount];
            } tables[AldoNtCount];
            struct {
                uint8_t x, y;
                bool h, v;
            } pos;
            bool pt;
        } nt;
        struct {
            struct {
                uint8_t x, y, tile, palette;
                bool pt, priority, hflip, vflip;
            } objects[AldoSpriteCount];
            bool double_height;
        } sprites;
        // A Pattern Table is 256 tiles x 8 rows x 8 pixels x 2 bits.
        struct {
            uint16_t
                left[AldoPtTileCount][AldoChrTileDim],
                right[AldoPtTileCount][AldoChrTileDim];
        } pattern_tables;
        // Background/Foreground, 4 Palettes, 4 Colors, 6 Bits
        struct {
            uint8_t bg[AldoPalSize][AldoPalSize],
                    fg[AldoPalSize][AldoPalSize];
        } palettes;
        bool newframe;
    } *video;
};

// if returns false then errno is set due to failed allocation
aldo_export aldo_checkerr
bool aldo_snapshot_extend(struct aldo_snapshot *snp) aldo_nothrow;
aldo_export
void aldo_snapshot_cleanup(struct aldo_snapshot *snp) aldo_nothrow;
#include "bridgeclose.h"

#endif
