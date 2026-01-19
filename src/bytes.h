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
#include <stdio.h>
#include <string.h>

// Memory-related Constants
enum {
    ALDO_BITWIDTH_1KB = 10,
    ALDO_BITWIDTH_2KB,
    ALDO_BITWIDTH_4KB,
    ALDO_BITWIDTH_8KB,
    ALDO_BITWIDTH_16KB,
    ALDO_BITWIDTH_32KB,
    ALDO_BITWIDTH_64KB,

    ALDO_MEMBLOCK_1KB = 1 << ALDO_BITWIDTH_1KB,
    ALDO_ADDRMASK_1KB = ALDO_MEMBLOCK_1KB - 1,

    ALDO_MEMBLOCK_2KB = 1 << ALDO_BITWIDTH_2KB,
    ALDO_ADDRMASK_2KB = ALDO_MEMBLOCK_2KB - 1,

    ALDO_MEMBLOCK_4KB = 1 << ALDO_BITWIDTH_4KB,
    ALDO_ADDRMASK_4KB = ALDO_MEMBLOCK_4KB - 1,

    ALDO_MEMBLOCK_8KB = 1 << ALDO_BITWIDTH_8KB,
    ALDO_ADDRMASK_8KB = ALDO_MEMBLOCK_8KB - 1,

    ALDO_MEMBLOCK_16KB = 1 << ALDO_BITWIDTH_16KB,
    ALDO_ADDRMASK_16KB = ALDO_MEMBLOCK_16KB - 1,

    ALDO_MEMBLOCK_32KB = 1 << ALDO_BITWIDTH_32KB,
    ALDO_ADDRMASK_32KB = ALDO_MEMBLOCK_32KB - 1,

    ALDO_MEMBLOCK_64KB = 1 << ALDO_BITWIDTH_64KB,
    ALDO_ADDRMASK_64KB = ALDO_MEMBLOCK_64KB - 1,

    ALDO_CPU_VECTOR_NMI = 0xfffa,
    ALDO_CPU_VECTOR_RST = 0xfffc,
    ALDO_CPU_VECTOR_IRQ = 0xfffe,
};

// Extract bit @pos from unsigned value
#define aldo_getbit(uval, pos) (((uval) >> (pos)) & 0x1)

// all mem macros require array expressions, not pointers
#define aldo_arrsz(arr) (sizeof (arr) / sizeof (arr)[0])
#define aldo_memclr(mem) memset(mem, 0, aldo_arrsz(mem))
#define aldo_memdump(mem, f) \
(fwrite(mem, sizeof (mem)[0], aldo_arrsz(mem), f) == aldo_arrsz(mem))

#include "bridgeopen.h"
// Convert unsigned values into little-endian byte representations;
// undefined behavior if any pointer/array arguments are null.

//
// MARK: - Export
//

// Bytes to Word
aldo_export
inline uint16_t aldo_bytowr(uint8_t lo, uint8_t hi) aldo_nothrow
{
    return (uint16_t)(lo | hi << 8);
}

// Byte Array to Word
aldo_export
inline uint16_t aldo_batowr(const uint8_t bytes[aldo_cz(2)]) aldo_nothrow
{
    return aldo_bytowr(bytes[0], bytes[1]);
}

//
// MARK: - Internal
//

// Word to Bytes
inline void aldo_wrtoby(uint16_t word, uint8_t *aldo_noalias lo,
                        uint8_t *aldo_noalias hi) aldo_nothrow
{
    *lo = (uint8_t)word;
    *hi = (uint8_t)(word >> 8);
}

// Word to Byte Array
inline void aldo_wrtoba(uint16_t word, uint8_t bytes[aldo_cz(2)]) aldo_nothrow
{
    aldo_wrtoby(word, bytes, bytes + 1);
}

// DWord to Byte Array
inline void aldo_dwtoba(uint32_t dword, uint8_t bytes[aldo_cz(4)]) aldo_nothrow
{
    for (size_t i = 0; i < 4; ++i) {
        bytes[i] = (uint8_t)(dword >> (8 * i));
    }
}

// Interleave the bits of lo and hi bytes together;
// Outer Perfect Shuffle algorithm taken from Hacker's Delight 2nd Edition §7-2
// and adapted to 16-bits; I *think* this is related to Morton Codes but
// I'm not entirely sure: https://en.wikipedia.org/wiki/Z-order_curve
inline uint16_t aldo_byteshuffle(uint8_t lo, uint8_t hi) aldo_nothrow
{
    // magic shuffle numbers
    static constexpr uint16_t m[] = {
        0x00f0, 0xf00f, 0x0c0c, 0xc3c3, 0x2222, 0x9999,
    };
    auto x = aldo_bytowr(lo, hi);
    x = (uint16_t)((x & m[0]) << 4 | ((x >> 4) & m[0]) | (x & m[1]));
    x = (uint16_t)((x & m[2]) << 2 | ((x >> 2) & m[2]) | (x & m[3]));
    x = (uint16_t)((x & m[4]) << 1 | ((x >> 1) & m[4]) | (x & m[5]));
    return x;
}

// Copy single bank @addr into destination buffer, assumes bank is sized to
// power-of-2 KB boundary between [1, 64].
size_t aldo_bytecopy_bank(const uint8_t *aldo_noalias bankmem, int bankwidth,
                          uint16_t addr, size_t count,
                          uint8_t dest[aldo_naz(count)]) aldo_nothrow;
#include "bridgeclose.h"

#endif
