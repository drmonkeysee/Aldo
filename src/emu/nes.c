//
//  nes.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "nes.h"

#include "cpu.h"

#include <assert.h>
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
    nes *self = malloc(sizeof *self);
    cpu_powerup(&self->cpu);
    return self;
}

void nes_free(nes *self)
{
    free(self);
}

void nes_snapshot(nes *self, struct console_state *snapshot)
{
    assert(self != NULL);
    if (!snapshot) return;

    cpu_snapshot(&self->cpu, snapshot);
}
