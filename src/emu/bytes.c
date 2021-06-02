//
//  bytes.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#include "bytes.h"

#include <assert.h>
#include <string.h>

extern inline uint16_t bytowr(uint8_t, uint8_t),
                       batowr(const uint8_t[static 2]);
extern inline void wrtoby(uint16_t, uint8_t *restrict, uint8_t *restrict),
                   wrtoba(uint16_t, uint8_t[static 2]);

size_t bytescopy_bank(const uint8_t *restrict bankmem, int bankwidth,
                      uint16_t addr, size_t count,
                      uint8_t dest[restrict count])
{
    assert(bankmem != NULL);
    assert(dest != NULL);
    assert(BITWIDTH_1KB <= bankwidth && bankwidth <= BITWIDTH_64KB);

    const size_t banksize = 1 << bankwidth;
    // NOTE: addr -> index is always mask(banksize - 1)
    // iff banksize is a power of 2
    const uint16_t start = addr & (banksize - 1);
    const size_t bytesleft = banksize - start,
                 bytecount = count > bytesleft ? bytesleft : count;
    memcpy(dest, bankmem + start, bytecount * sizeof *dest);
    return bytecount;
}

size_t bytecopy_bankmirrored(const uint8_t *restrict bankmem, int bankwidth,
                             uint16_t addr, int addrwidth, size_t count,
                             uint8_t dest[restrict count])
{
    assert(bankmem != NULL);
    assert(dest != NULL);
    assert(BITWIDTH_1KB <= bankwidth && bankwidth <= BITWIDTH_64KB);
    assert(BITWIDTH_1KB <= addrwidth && addrwidth <= BITWIDTH_64KB);
    assert(bankwidth < addrwidth);

    const size_t banksize = 1 << bankwidth,
                 addrspace = 1 << addrwidth;
    assert(addrspace > addr);

    // NOTE: addr -> index is always mask(banksize - 1)
    // iff banksize is a power of 2
    uint16_t bankstart = addr & (banksize - 1);
    const size_t spaceleft = addrspace - addr,
                 maxcount = count > spaceleft ? spaceleft : count,
                 bankleft = banksize - bankstart;
    size_t bytescopy = count > bankleft ? bankleft : count;
    ptrdiff_t bytesleft = maxcount;
    // NOTE: Mirrored bank bytecopy needs to:
    // 1) start copy from some offset within the bank or a bank mirror
    // 2) wraparound if copy crosses bank boundary within address space
    // 3) stop copy at end of address space
    do {
        memcpy(dest, bankmem + bankstart, bytescopy * sizeof *dest);
        // NOTE: first memcpy block may be offset from start of bank; any
        // additional blocks start at 0 due to mirroring-wraparound.
        bankstart = 0;
        bytesleft -= bytescopy;
        dest += bytescopy;
        bytescopy = bytesleft > (ptrdiff_t)banksize ? banksize : bytesleft;
    } while (bytesleft > 0);

    // NOTE: if we went negative our math is wrong
    assert(bytesleft == 0);
    return maxcount;
}
