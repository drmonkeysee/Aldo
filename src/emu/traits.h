//
//  traits.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_emu_traits_h
#define Aldo_emu_traits_h

#include <stdint.h>

// Bit Width Constants
enum {
    BITWIDTH_1KB = 10,
    BITWIDTH_2KB = 11,
    BITWIDTH_8KB = 13,
    BITWIDTH_16KB = 14,
    BITWIDTH_32KB = 15,
    BITWIDTH_64KB = 16,
};

// CPU Memory Map
#define NES_RAM_SIZE 0x800u     // 2 KB CPU RAM
#define NES_ROM_SIZE 0x8000u    // 32 KB Fake ROM
extern const uint16_t
    CpuRamMaxAddr, CpuRamAddrMask, CpuRomMinAddr, CpuRomMaxAddr,
    CpuRomAddrMask,

// Interrupt Vectors
    NmiVector, ResetVector, IrqVector;

// Instruction Characteristics
extern const int MaxCycleCount;

#endif
