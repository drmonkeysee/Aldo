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
    SNP_PAL_SIZE = 4,
    SNP_PAT_TILES = 256,
    // NOTE: CHR bit-planes are 8 bits wide and 8 bytes tall; a CHR tile is a
    // 16-byte array of 2-bit palette-indexed pixels composed of two bit-planes
    // where the first plane specifies the pixel low bit and the second plane
    // specifies the pixel high bit.
    SNP_TILE_DIM = 8,
    SNP_TILE_STRIDE = 2 * SNP_TILE_DIM,
};

struct snapshot {
    struct {
        uint16_t program_counter;
        uint8_t accumulator, stack_pointer, status, xindex, yindex;
        struct {
            bool irq, nmi, readwrite, ready, reset, sync;
        } lines;
        struct {
            enum csig_state irq, nmi, rst;
            uint16_t addressbus, current_instruction;
            uint8_t addrlow_latch, addrhigh_latch, addrcarry_latch, databus,
                    exec_cycle, opcode;
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
            enum csig_state rst;
            uint16_t addressbus, scrolladdr, tempaddr, dot, line;
            uint8_t databus, readbuffer, register_databus, register_select,
                    xfine;
            bool busfault, cv_pending, oddframe, writelatch;
        } datapath;
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
        struct snpprg {
            size_t length;      // Number of bytes copied to pc
            uint8_t pc[96];     // 32 lines @ max 3-byte instructions
        } *curr;
    } prg;

    struct {
        // A Pattern Table is 256 tiles x 8 rows x 8 pixels x 2 bits.
        struct {
            uint16_t
                left[SNP_PAT_TILES][SNP_TILE_DIM],
                right[SNP_PAT_TILES][SNP_TILE_DIM];
        } pattern_tables;
        // Background/Foreground, 4 Palettes, 4 Colors, 6 Bits
        struct {
            uint8_t bg[SNP_PAL_SIZE][SNP_PAL_SIZE],
                    fg[SNP_PAL_SIZE][SNP_PAL_SIZE];
        } palettes;
    } *video;
};

#include "bridgeopen.h"
br_libexport
void snapshot_extend(struct snapshot *snp) br_nothrow;
br_libexport
void snapshot_cleanup(struct snapshot *snp) br_nothrow;
#include "bridgeclose.h"

#endif
