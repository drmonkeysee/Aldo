//
//  ppu.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#ifndef Aldo_ppu_h
#define Aldo_ppu_h

#include "bus.h"

// The Ricoh RP2C02 Picture Processing Unit (PPU) is a
// fixed-function IC that generates the NES video signal.
struct rp2c02 {
    // Main Bus: connected to the CPU
    bus *mbus;  // Non-owning Pointer
    // Video Bus: video components connected to the PPU
    bus *vbus;  // Non-owning Pointer
};

#endif
