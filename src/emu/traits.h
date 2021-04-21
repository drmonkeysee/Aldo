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

enum cpumem {
    CMEM_NONE,
    CMEM_RAM,
    CMEM_ROM,
};

inline enum cpumem addr_to_cpumem(uint16_t addr)
{
    if (addr <= CpuRamMaxAddr) return CMEM_RAM;
    if (CpuCartMinAddr <= addr && addr <= CpuCartMaxAddr) return CMEM_ROM;
    return CMEM_NONE;
}

// Instruction Characteristics
extern const int MaxCycleCount;

#endif
