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

struct snapshot {
    struct {
        ptrdiff_t halted;
    } debugger;

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

    struct {
        size_t length;          // Number of bytes copied to currprg
        uint8_t curr[96],       // 32 lines @ max 3-byte instructions
                vectors[6];
    } prg;

    struct {
        // 2 Tables, 256 Tiles, 8 Rows, 16 Bits
        // TODO: don't write this every update?
        uint16_t pattern_tables[2][256][8];
        // Background/Foreground, 4 Palettes, 4 Colors, 6 Bits
        uint8_t bgpalettes[4][4], fgpalettes[4][4];
    } *video;
};

#include "bridgeopen.h"
br_libexport
void snapshot_extsetup(struct snapshot *snp) br_nothrow;
br_libexport
void snapshot_extcleanup(struct snapshot *snp) br_nothrow;
#include "bridgeclose.h"

#endif
