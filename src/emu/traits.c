//
//  traits.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/21/21.
//

#include "traits.h"

const uint16_t
    CpuRamMaxAddr = (RAM_SIZE * 4) - 1, // Max CPU addr is 2 KB * 4
                                        // due to mirroring.
    CpuRamAddrMask = RAM_SIZE - 1,      // Mask off everything above 2 KB
    CpuRomMinAddr = ROM_SIZE,           // Fake ROM starts at $8000
    CpuRomMaxAddr = UINT16_MAX,         // and ends at $FFFF.
    CpuRomAddrMask = ROM_SIZE - 1,      // [$8000, $FFFF] -> [0x0000, 0x7fff]

    NmiVector = 0xfffa,
    ResetVector = 0xfffc,
    IrqVector = 0xfffe;

const int MaxCycleCount = 7;
