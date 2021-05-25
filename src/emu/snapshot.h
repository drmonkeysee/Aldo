//
//  snapshot.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/16/21.
//

#ifndef Aldo_emu_snapshot_h
#define Aldo_emu_snapshot_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SNP_PRGV_SIZE 192   // NOTE: max 64 lines @ 3-byte instructions

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
    // TODO: remove these
    const uint8_t *ram, *rom;
    struct {
        uint8_t *ram,
                prgview[SNP_PRGV_SIZE],
                vectors[6];
        size_t prglength;   // Number of bytes copied to prgview this frame
    } mem;
    struct {
        const char *formatname;
    } cart;
    enum nexcmode mode;
    struct {
        enum nistate irq, nmi, res;
        uint16_t addressbus, current_instruction;
        uint8_t addrlow_latch, addrhigh_latch, addrcarry_latch, databus,
                exec_cycle, opcode;
        bool busfault, instdone;
    } datapath;
    struct {
        uint16_t program_counter;
        uint8_t accumulator, stack_pointer, status, xindex, yindex;
    } cpu;
    struct {
        bool irq, nmi, readwrite, ready, reset, sync;
    } lines;
};

#endif
