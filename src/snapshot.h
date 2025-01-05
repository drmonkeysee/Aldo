//
//  snapshot.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/16/21.
//

#ifndef Aldo_snapshot_h
#define Aldo_snapshot_h

#include "ctrlsignal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    ALDO_PAL_SIZE = 4,
    // NOTE: CHR bit-planes are 8 bits wide and 8 bytes tall; a CHR tile is a
    // 16-byte array of 2-bit palette-indexed pixels composed of two bit-planes
    // where the first plane specifies the pixel low bit and the second plane
    // specifies the pixel high bit.
    ALDO_CHR_TILE_DIM = 8,
    ALDO_CHR_TILE_STRIDE = 2 * ALDO_CHR_TILE_DIM,
    ALDO_PT_TILE_COUNT = 256,
    ALDO_NT_WIDTH = 32,
    ALDO_NT_HEIGHT = 30,
    ALDO_NT_COUNT = ALDO_NT_WIDTH * ALDO_NT_HEIGHT,
    ALDO_ATTR_COUNT = 64,
};

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

    // NOTE: large fields below this point are managed via setup/cleanup to
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
        // TODO: include scroll position?
        struct {
            uint8_t attributes[ALDO_ATTR_COUNT],
                    palettes[ALDO_NT_COUNT],
                    tiles[ALDO_NT_COUNT];
        } nametables[2];
        // A Pattern Table is 256 tiles x 8 rows x 8 pixels x 2 bits.
        struct {
            uint16_t
                left[ALDO_PT_TILE_COUNT][ALDO_CHR_TILE_DIM],
                right[ALDO_PT_TILE_COUNT][ALDO_CHR_TILE_DIM];
        } pattern_tables;
        // Background/Foreground, 4 Palettes, 4 Colors, 6 Bits
        struct {
            uint8_t bg[ALDO_PAL_SIZE][ALDO_PAL_SIZE],
                    fg[ALDO_PAL_SIZE][ALDO_PAL_SIZE];
        } palettes;
        bool currpt, newframe;
    } *video;
};

#include "bridgeopen.h"
// NOTE: if returns false then errno is set due to failed allocation
br_libexport br_checkerror
bool aldo_snapshot_extend(struct aldo_snapshot *snp) br_nothrow;
br_libexport
void aldo_snapshot_cleanup(struct aldo_snapshot *snp) br_nothrow;
#include "bridgeclose.h"

#endif
