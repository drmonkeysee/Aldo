//
//  bytes.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#ifndef Aldo_bytes_h
#define Aldo_bytes_h

#include <stddef.h>
#include <stdint.h>

// Memory-related Constants
enum {
    BITWIDTH_256B = 8,
    BITWIDTH_512B,
    BITWIDTH_1KB,
    BITWIDTH_2KB,
    BITWIDTH_4KB,
    BITWIDTH_8KB,
    BITWIDTH_16KB,
    BITWIDTH_32KB,
    BITWIDTH_64KB,

    MEMBLOCK_256B = 1 << BITWIDTH_256B,
    ADDRMASK_256B = MEMBLOCK_256B - 1,

    MEMBLOCK_512B = 1 << BITWIDTH_512B,
    ADDRMASK_512B = MEMBLOCK_512B - 1,

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
    CPU_VECTOR_RST = 0xfffc,
    CPU_VECTOR_IRQ = 0xfffe,
};

// Extract bit @pos from byte
#define byte_getbit(byte, pos) (((byte) >> (pos)) & 1)

#include "bridgeopen.h"
// NOTE: convert unsigned values into little-endian byte representations;
// undefined behavior if any pointer/array arguments are null.

//
// Export
//

// Bytes to Word
br_libexport
inline uint16_t bytowr(uint8_t lo, uint8_t hi)
{
    return (uint16_t)(lo | hi << 8);
}

// Byte Array to Word
br_libexport
inline uint16_t batowr(const uint8_t bytes[br_csz(2)])
{
    return bytowr(bytes[0], bytes[1]);
}

//
// Internal
//

// Word to Bytes
inline void wrtoby(uint16_t word, uint8_t *br_noalias lo,
                   uint8_t *br_noalias hi)
{
    *lo = (uint8_t)(word & 0xff);
    *hi = (uint8_t)(word >> 8);
}

// Word to Byte Array
inline void wrtoba(uint16_t word, uint8_t bytes[br_csz(2)])
{
    wrtoby(word, bytes, bytes + 1);
}

// DWord to Byte Array
inline void dwtoba(uint32_t dword, uint8_t bytes[br_csz(4)])
{
    for (size_t i = 0; i < 4; ++i) {
        bytes[i] = (dword >> (8 * i)) & 0xff;
    }
}

// Copy single bank @addr into destination buffer, assumes bank is sized to
// power-of-2 KB boundary between [1, 64].
size_t bytecopy_bank(const uint8_t *br_noalias bankmem, int bankwidth,
                     uint16_t addr, size_t count,
                     uint8_t dest[br_noalias_sz(count)]);
#include "bridgeclose.h"

#endif
