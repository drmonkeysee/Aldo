//
//  traits.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_emu_traits_h
#define Aldo_emu_traits_h

#include <stdint.h>

// CPU Memory Map
#define RAM_SIZE 0x800u
extern const uint16_t CpuRamMaxAddr, CpuRamAddrMask, CpuCartMinAddr,
                      CpuCartMaxAddr, CpuCartAddrMask;

// Interrupt Vectors
extern const uint16_t NmiVector, ResetVector, IrqVector;

#endif
