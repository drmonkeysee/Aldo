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
    struct mos6502 cpu;     // CPU Core of RP2A03 Chip
    uint8_t ram[0x800],     // 2 KB CPU Internal RAM
            cart[0xbfe0];   // TODO: ~49 KB Cartridge RAM/ROM to be replaced
                            // eventually with cartridge + mapper
};

static void load_prog(nes *self, size_t sz, uint8_t prog[restrict sz])
{
    // TODO: stick test programs at 0x8000 for now
    static const uint16_t start = 0x8000;

    assert(sz <= 0xffff - start);

    for (size_t i = 0; i < sz; ++i) {
        self->cart[(i + start) & CART_CPU_ADDR_MASK] = prog[i];
    }
    self->cart[RESET_VECTOR & CART_CPU_ADDR_MASK] = (uint8_t)start;
    self->cart[(RESET_VECTOR + 1) & CART_CPU_ADDR_MASK]
        = (uint8_t)(start >> 8);
}

//
// Public Interface
//

nes *nes_new(void)
{
    struct nes_console *self = malloc(sizeof *self);
    self->cpu.ram = self->ram;
    self->cpu.cart = self->cart;
    return self;
}

void nes_free(nes *self)
{
    free(self);
}

void nes_powerup(nes *self, size_t sz, uint8_t prog[restrict sz])
{
    assert(self != NULL);

    load_prog(self, sz, prog);
    cpu_reset(&self->cpu);
}

void nes_snapshot(nes *self, struct console_state *snapshot)
{
    assert(self != NULL);
    if (!snapshot) return;

    cpu_snapshot(&self->cpu, snapshot);
    snapshot->ram = self->ram;
}
