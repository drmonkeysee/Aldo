//
//  snapshot.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/16/21.
//

#ifndef Aldo_emu_snapshot_h
#define Aldo_emu_snapshot_h

#include <stdbool.h>
#include <stdint.h>

struct console_state {
    const uint8_t *ram, *rom;
    struct {
        uint16_t addressbus, program_counter;
        uint8_t accumulator, addrlow_latch, databus, sequence_cycle, opcode,
                stack_pointer, status, xindex, yindex;
    } cpu;
    struct {
        bool irq, nmi, readwrite, ready, reset, sync;
    } lines;
};

#endif
