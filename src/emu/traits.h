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
    BITWIDTH_2KB,
    BITWIDTH_4KB,
    BITWIDTH_8KB,
    BITWIDTH_16KB,
    BITWIDTH_32KB,
    BITWIDTH_64KB,
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
