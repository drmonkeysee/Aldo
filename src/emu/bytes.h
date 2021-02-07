//
//  bytes.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#ifndef Aldo_emu_bytes_h
#define Aldo_emu_bytes_h

#include <stdint.h>

// NOTE: undefined behavior if any pointer arguments are null or have
// too few elements.

inline uint16_t byt_tow(uint8_t lo, uint8_t hi)
{
    return lo | (hi << 8);
}

inline uint16_t bya_tow(const uint8_t bytes[static 2])
{
    return byt_tow(bytes[0], bytes[1]);
}

inline void byt_frw(uint16_t word, uint8_t *restrict lo, uint8_t *restrict hi)
{
    *lo = word & 0xff;
    *hi = word >> 8;
}

inline void bya_frw(uint16_t word, uint8_t bytes[static 2])
{
    byt_frw(word, bytes, bytes + 1);
}

#endif
