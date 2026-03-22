//
//  chip.h
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#ifndef Aldo_chip_h
#define Aldo_chip_h

#include "cpu.h"

struct aldo_snapshot;

// The Ricoh RP2A03 Microprocessor; includes the 6502 CPU,
// Audio Processing Unit (APU), Direct Memory Access (DMA) units,
// and Joypad control.
struct aldo_rp2a03 {
    struct aldo_mos6502 cpu;    // 6502 CPU Core

    bool put;                   // Whether the current cycle is a DMA get or put;
                                // also used to count APU cycles per CPU cycles,
                                // 2 CPU Cycles = 1 APU Cycle.
};

void aldo_chip_powerup(struct aldo_rp2a03 *self);

int aldo_chip_cycle(struct aldo_rp2a03 *self);

void aldo_chip_snapshot(const struct aldo_rp2a03 *self, struct aldo_snapshot *snp);

#endif
