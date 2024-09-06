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
    CPU_VECTOR_RST = 0xfffc,
    CPU_VECTOR_IRQ = 0xfffe,
};

// Extract bit @pos from byte
#define byte_getbit(byte, pos) (((byte) >> (pos)) & 0x1)

#include "bridgeopen.h"
// NOTE: convert unsigned values into little-endian byte representations;
// undefined behavior if any pointer/array arguments are null.

//
// MARK: - Export
//

// Bytes to Word
br_libexport
inline uint16_t bytowr(uint8_t lo, uint8_t hi) br_nothrow
{
    return (uint16_t)(lo | hi << 8);
}

// Byte Array to Word
br_libexport
inline uint16_t batowr(const uint8_t bytes[br_csz(2)]) br_nothrow
{
    return bytowr(bytes[0], bytes[1]);
}

//
// MARK: - Internal
//

// Word to Bytes
inline void wrtoby(uint16_t word, uint8_t *br_noalias lo,
                   uint8_t *br_noalias hi) br_nothrow
{
    *lo = (uint8_t)(word & 0xff);
    *hi = (uint8_t)(word >> 8);
}

// Word to Byte Array
inline void wrtoba(uint16_t word, uint8_t bytes[br_csz(2)]) br_nothrow
{
    wrtoby(word, bytes, bytes + 1);
}

// DWord to Byte Array
inline void dwtoba(uint32_t dword, uint8_t bytes[br_csz(4)]) br_nothrow
{
    for (size_t i = 0; i < 4; ++i) {
        bytes[i] = (dword >> (8 * i)) & 0xff;
    }
}

// Interleave the bits of lo and hi bytes together;
// Outer Perfect Shuffle algorithm taken from Hacker's Delight 2nd Edition §7-2
// and adapted to 16-bits; I *think* this is related to Morton Codes but
// I'm not entirely sure: https://en.wikipedia.org/wiki/Z-order_curve
inline uint16_t shuffle(uint8_t lo, uint8_t hi) br_nothrow
{
    // NOTE: magic shuffle numbers
    static const uint16_t m[] = {
        0x00f0, 0xf00f, 0x0c0c, 0xc3c3, 0x2222, 0x9999,
    };
    uint16_t x = bytowr(lo, hi);
    x = (uint16_t)((x & m[0]) << 4 | ((x >> 4) & m[0]) | (x & m[1]));
    x = (uint16_t)((x & m[2]) << 2 | ((x >> 2) & m[2]) | (x & m[3]));
    x = (uint16_t)((x & m[4]) << 1 | ((x >> 1) & m[4]) | (x & m[5]));
    return x;
}

// Copy single bank @addr into destination buffer, assumes bank is sized to
// power-of-2 KB boundary between [1, 64].
size_t bytecopy_bank(const uint8_t *br_noalias bankmem, int bankwidth,
                     uint16_t addr, size_t count,
                     uint8_t dest[br_noalias_sz(count)]) br_nothrow;
#include "bridgeclose.h"

#endif
