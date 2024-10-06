//
//  bytes.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#include "bytes.h"

#include <assert.h>
#include <string.h>

extern inline uint16_t aldo_bytowr(uint8_t, uint8_t),
                       aldo_batowr(const uint8_t[static 2]),
                       aldo_byteshuffle(uint8_t, uint8_t);
extern inline void aldo_wrtoby(uint16_t, uint8_t *restrict, uint8_t *restrict),
                   aldo_wrtoba(uint16_t, uint8_t[static 2]),
                   aldo_dwtoba(uint32_t, uint8_t[static 4]);

size_t aldo_bytecopy_bank(const uint8_t *restrict bankmem, int bankwidth,
                          uint16_t addr, size_t count,
                          uint8_t dest[restrict count])
{
    assert(bankmem != NULL);
    assert(dest != NULL);
    assert(ALDO_BITWIDTH_1KB <= bankwidth && bankwidth <= ALDO_BITWIDTH_64KB);

    size_t banksize = 1 << bankwidth;
    // NOTE: addr -> index is always mask(banksize - 1)
    // iff banksize is a power of 2
    uint16_t start = addr & (uint16_t)(banksize - 1);
    size_t
        bytesleft = banksize - start,
        bytecount = count > bytesleft ? bytesleft : count;
    memcpy(dest, bankmem + start, bytecount * sizeof *dest);
    return bytecount;
}
