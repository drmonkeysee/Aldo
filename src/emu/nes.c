//
//  nes.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "nes.h"

#include "cpu.h"

#include <stddef.h>
#include <stdlib.h>

int nes_rand(void)
{
    return rand();
}

struct nes_console {
    struct mos6502 cpu;
};

//
// Public Interface
//

nes *nes_new(void)
{
    return NULL;
}

void nes_free(nes *self)
{
    (void)self;
}
