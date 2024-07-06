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
        const uint8_t *ram;     // Non-owning Pointer
        size_t prglength;       // Number of bytes copied to currprg
        uint8_t currprg[192],   // 64 lines @ max 3-byte instructions
                vectors[6];
    } mem;
    struct {
        ptrdiff_t halted;
    } debugger;
    struct {
        enum csig_state irq, nmi, rst;
        uint16_t addressbus, current_instruction;
        uint8_t addrlow_latch, addrhigh_latch, addrcarry_latch, databus,
                exec_cycle, opcode;
        bool busfault, instdone, jammed;
    } datapath;
    struct {
        enum csig_state rst;
        uint16_t addressbus, curraddr, tempaddr, dot, line;
        uint8_t databus, readbuffer, register_databus, register_select, xfine;
        bool cv_pending, oddframe, writelatch;
    } pdatapath;
    struct {
        uint16_t program_counter;
        uint8_t accumulator, stack_pointer, status, xindex, yindex;
    } cpu;
    struct {
        uint8_t ctrl, mask, oamaddr, status;
    } ppu;
    struct {
        bool irq, nmi, readwrite, ready, reset, sync;
    } lines;
    struct {
        bool
            address_enable, cpu_readwrite, interrupt, read, reset, video_out,
            write;
    } plines;
};

#include "bridgeopen.h"
br_libexport
void snapshot_clear(struct snapshot *snp) br_nothrow;
#include "bridgeclose.h"

#endif
