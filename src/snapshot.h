//
//  snapshot.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/16/21.
//

#ifndef Aldo_snapshot_h
#define Aldo_snapshot_h

#include "haltexpr.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum nexcmode {
    NEXC_CYCLE,
    NEXC_STEP,
    NEXC_RUN,
    NEXC_MODECOUNT,
};

enum nistate {
    NIS_CLEAR,
    NIS_DETECTED,
    NIS_PENDING,
    NIS_COMMITTED,
    NIS_SERVICED,
};

struct console_state {
    struct {
        int format;
        uint8_t mapid;
    } cart;
    struct {
        const uint8_t *ram;     // Non-owning Pointer
        size_t prglength;       // Number of bytes copied to prgview
        uint8_t prgview[192],   // 64 lines @ max 3-byte instructions
                vectors[6];
    } mem;
    struct {
        int resvector_override; // RESET Vector Override (<0 if not set)
        struct haltexpr break_condition;
    } debugger;
    enum nexcmode mode;
    struct {
        enum nistate irq, nmi, res;
        uint16_t addressbus, current_instruction;
        uint8_t addrlow_latch, addrhigh_latch, addrcarry_latch, databus,
                exec_cycle, opcode;
        bool busfault, instdone, jammed;
    } datapath;
    struct {
        uint16_t program_counter;
        uint8_t accumulator, stack_pointer, status, xindex, yindex;
        bool bcd_support;
    } cpu;
    struct {
        bool irq, nmi, readwrite, ready, reset, sync;
    } lines;
};

#endif
