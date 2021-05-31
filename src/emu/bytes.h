//
//  bytes.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#ifndef Aldo_emu_bytes_h
#define Aldo_emu_bytes_h

#include <stddef.h>
#include <stdint.h>

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

// NOTE: undefined behavior if any pointer/array arguments are null

// Bytes to Word
inline uint16_t bytowr(uint8_t lo, uint8_t hi)
{
    return lo | (hi << 8);
}

// Byte Array to Word
inline uint16_t batowr(const uint8_t bytes[static 2])
{
    return bytowr(bytes[0], bytes[1]);
}

// Word to Bytes
inline void wrtoby(uint16_t word, uint8_t *restrict lo, uint8_t *restrict hi)
{
    *lo = word & 0xff;
    *hi = word >> 8;
}

// Word to Byte Array
inline void wrtoba(uint16_t word, uint8_t bytes[static 2])
{
    wrtoby(word, bytes, bytes + 1);
}

// Copy single bank @addr into destination buffer, assumes bank is sized to
// power-of-2 KB boundary between [1, 64].
size_t bytescopy_bank(const uint8_t *restrict bankmem, int bankwidth,
                      uint16_t addr, size_t count,
                      uint8_t dest[restrict count]);

// Copy mirrored banks @addr into destination buffer; assumes banks and address
// spaces are sized to power-of-2 KB boundaries between [1, 64].
size_t bytecopy_bankmirrored(const uint8_t *restrict bankmem, int banksize,
                             uint16_t addr, int addrspace, size_t count,
                             uint8_t dest[restrict count]);

#endif
