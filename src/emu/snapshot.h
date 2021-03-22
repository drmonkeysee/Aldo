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

enum nexcmode {
    NEXC_CYCLE,
    NEXC_STEP,
    NEXC_RUN,
    NEXC_MODECOUNT,
};

struct console_state {
    const uint8_t *ram, *rom;
    enum nexcmode mode;
    struct {
        uint16_t addressbus, program_counter;
        uint8_t accumulator, addra_latch, addrb_latch, addrc_latch, databus,
                exec_cycle, opcode, stack_pointer, status, xindex, yindex;
        bool datafault, instdone;
    } cpu;
    struct {
        bool irq, nmi, readwrite, ready, reset, sync;
    } lines;
};

#endif
