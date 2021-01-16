//
//  traits.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_emu_traits_h
#define Aldo_emu_traits_h

#include <stdint.h>

struct console_state {
    uint16_t program_counter;
    uint8_t accum, stack_pointer, status, xindex, yindex;
    const uint8_t *ram;
};

#endif
