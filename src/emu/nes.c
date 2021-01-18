//
//  nes.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "nes.h"

#include "cpu.h"
#include "traits.h"

#include <assert.h>
#include <stdlib.h>

int nes_rand(void)
{
    return rand();
}

// NOTE: The NES-001 Motherboard

struct nes_console {
    struct mos6502 cpu; // CPU Core of RP2A03 Chip
    uint8_t ram[0x800]; // 2 KB CPU Internal RAM
};

//
// Public Interface
//

nes *nes_new(void)
{
    struct nes_console *self = malloc(sizeof *self);
    self->cpu.ram = self->ram;
    return self;
}

void nes_free(nes *self)
{
    free(self);
}

void nes_powerup(nes *self, size_t sz, uint8_t prog[restrict sz])
{
    static const uint16_t start = 0x700;

    cpu_reset(&self->cpu);
    // TODO: temporarily put programs at $0700
    for (size_t i = 0; i < sz; ++i) {
        self->ram[i + start] = prog[i];
    }
}

void nes_snapshot(nes *self, struct console_state *snapshot)
{
    assert(self != NULL);
    if (!snapshot) return;

    cpu_snapshot(&self->cpu, snapshot);
    snapshot->ram = self->ram;
}
