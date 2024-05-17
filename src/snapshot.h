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

struct console_state {
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
        enum csig_state irq, nmi, res;
        uint16_t addressbus, current_instruction;
        uint8_t addrlow_latch, addrhigh_latch, addrcarry_latch, databus,
                exec_cycle, opcode;
        bool busfault, instdone, jammed;
    } datapath;
    struct {
        uint16_t program_counter;
        uint8_t accumulator, stack_pointer, status, xindex, yindex;
    } cpu;
    struct {
        bool irq, nmi, readwrite, ready, reset, sync;
    } lines;
    struct {
        int dot, line;
        uint8_t register_databus;
    } ppu;
};

#include "bridgeopen.h"
br_libexport
void snapshot_clear(struct console_state *snapshot) br_nothrow;
#include "bridgeclose.h"

#endif
