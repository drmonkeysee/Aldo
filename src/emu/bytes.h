//
//  bytes.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#ifndef Aldo_emu_bytes_h
#define Aldo_emu_bytes_h

#include <stdint.h>

// NOTE: undefined behavior if any pointer/array arguments are null

inline uint16_t bytowr(uint8_t lo, uint8_t hi)
{
    return lo | (hi << 8);
}

inline uint16_t batowr(const uint8_t bytes[static 2])
{
    return bytowr(bytes[0], bytes[1]);
}

inline void wrtoby(uint16_t word, uint8_t *restrict lo, uint8_t *restrict hi)
{
    *lo = word & 0xff;
    *hi = word >> 8;
}

inline void wrtoba(uint16_t word, uint8_t bytes[static 2])
{
    wrtoby(word, bytes, bytes + 1);
}

#endif
