//
//  apu.h
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#ifndef Aldo_apu_h
#define Aldo_apu_h

#include "cpu.h"

// The Ricoh RP2A03 Chip; includes the 6502 CPU, Audio Processing Unit (APU),
// Direct Memory Access (DMA) units, and Joypad control.
struct aldo_rp2a03 {
    struct aldo_mos6502 cpu;
};

#endif
