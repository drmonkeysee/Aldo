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
#define RAM_SIZE 0x800u     // 2 KB CPU RAM
#define ROM_SIZE 0x8000u    // 32 KB Fake ROM
extern const uint16_t
    CpuRamMaxAddr, CpuRamAddrMask, CpuCartMinAddr, CpuCartMaxAddr,
    CpuCartAddrMask,

// Interrupt Vectors
    NmiVector, ResetVector, IrqVector;

#endif
