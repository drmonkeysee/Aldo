//
//  traits.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_emu_traits_h
#define Aldo_emu_traits_h

// Memory-related Constants
enum {
    BITWIDTH_1KB = 10,
    BITWIDTH_2KB,
    BITWIDTH_4KB,
    BITWIDTH_8KB,
    BITWIDTH_16KB,
    BITWIDTH_32KB,
    BITWIDTH_64KB,

    MEMBLOCK_1KB = 1 << BITWIDTH_1KB,
    ADDRMASK_1KB = MEMBLOCK_1KB - 1,

    MEMBLOCK_2KB = 1 << BITWIDTH_2KB,
    ADDRMASK_2KB = MEMBLOCK_2KB - 1,

    MEMBLOCK_4KB = 1 << BITWIDTH_4KB,
    ADDRMASK_4KB = MEMBLOCK_4KB - 1,

    MEMBLOCK_8KB = 1 << BITWIDTH_8KB,
    ADDRMASK_8KB = MEMBLOCK_8KB - 1,

    MEMBLOCK_16KB = 1 << BITWIDTH_16KB,
    ADDRMASK_16KB = MEMBLOCK_16KB - 1,

    MEMBLOCK_32KB = 1 << BITWIDTH_32KB,
    ADDRMASK_32KB = MEMBLOCK_32KB - 1,

    MEMBLOCK_64KB = 1 << BITWIDTH_64KB,
    ADDRMASK_64KB = MEMBLOCK_64KB - 1,

    CPU_VECTOR_NMI = 0xfffa,
    CPU_VECTOR_RES = 0xfffc,
    CPU_VECTOR_IRQ = 0xfffe,
};

// Instruction Characteristics
extern const int MaxCycleCount;

#endif
