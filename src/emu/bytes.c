//
//  bytes.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#include "bytes.h"

extern inline uint16_t bytowr(uint8_t, uint8_t);
extern inline uint16_t batowr(const uint8_t[static 2]);
extern inline void wrtoby(uint16_t, uint8_t *restrict, uint8_t *restrict);
extern inline void wrtoba(uint16_t, uint8_t[static 2]);
