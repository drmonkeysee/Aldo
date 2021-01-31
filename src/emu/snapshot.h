//
//  snapshot.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/16/21.
//

#ifndef Aldo_emu_snapshot_h
#define Aldo_emu_snapshot_h

#include <stdint.h>

struct console_state {
    uint16_t program_counter;
    uint8_t accum, stack_pointer, status, xindex, yindex;
    const uint8_t *ram, *rom;
};

#endif
