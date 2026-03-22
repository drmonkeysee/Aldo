//
//  proc.h
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#ifndef Aldo_proc_h
#define Aldo_proc_h

#include "cpu.h"

// The Ricoh RP2A03 Microprocessor; includes the 6502 CPU,
// Audio Processing Unit (APU), Direct Memory Access (DMA) units,
// and Joypad control.
struct aldo_rp2a03 {
    struct aldo_mos6502 cpu;    // 6502 CPU Core

    bool put;                   // Whether the current cycle is a DMA get or put;
                                // also used to count APU cycles per CPU cycles,
                                // 2 CPU Cycles = 1 APU Cycle.
};

void aldo_proc_powerup(struct aldo_rp2a03 *self);

#endif
