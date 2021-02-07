//
//  bytes.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/6/21.
//

#include "bytes.h"

extern inline uint16_t bytes_to_word(uint8_t, uint8_t);
extern inline uint16_t bytea_to_word(const uint8_t[static 2]);
extern inline void word_to_bytes(uint16_t, uint8_t *restrict,
                                 uint8_t *restrict);
extern inline void word_to_bytea(uint16_t, uint8_t[static 2]);
