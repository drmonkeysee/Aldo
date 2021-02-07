//
//  bytes.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#include "bytes.h"

extern inline uint16_t byt_tow(uint8_t, uint8_t);
extern inline uint16_t bya_tow(const uint8_t[static 2]);
extern inline void byt_frw(uint16_t, uint8_t *restrict, uint8_t *restrict);
extern inline void bya_frw(uint16_t, uint8_t[static 2]);
