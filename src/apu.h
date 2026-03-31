//
//  apu.h
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#ifndef Aldo_apu_h
#define Aldo_apu_h

#include "cpu.h"
#include "ctrlsignal.h"

#include <stdint.h>

struct aldo_snapshot;

// The Ricoh RP2A03 Microprocessor; includes the 6502 CPU and auxiliary functions
// specific to the NES, the bulk of which is the Audio Processing Unit (APU),
// but also includes Direct Memory Access (DMA) units and Joypad control.
struct aldo_rp2a03 {
    struct aldo_mos6502 cpu;    // 6502 CPU Core

    struct {
        enum aldo_sigstate s;   // OAM DMA state
        uint8_t dma,            // OAMDMA register; DMA high address byte
                low;            // DMA low address byte
    } oam;

    // The RP2A03 has its own bus lines distinct from the 6502 core,
    // used by the DMA units.
    // TODO: there are bus conflicts involving DMC DMA and the CPU
    uint16_t addrbus;
    uint8_t databus;

    struct {
        bool rdy;               // Ready Signal (output); wired to CPU RDY
    } signal;

    bool put;                   // Whether the current cycle is a DMA get or put;
                                // also used to count APU cycles per CPU cycles,
                                // 2 CPU Cycles = 1 APU Cycle.
};

void aldo_apu_connect(struct aldo_rp2a03 *self);
void aldo_apu_powerup(struct aldo_rp2a03 *self);

int aldo_apu_cycle(struct aldo_rp2a03 *self);

void aldo_apu_snapshot(const struct aldo_rp2a03 *self, struct aldo_snapshot *snp);

#endif
