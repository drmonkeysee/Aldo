//
//  nes.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "nes.h"

#include "bytes.h"
#include "cpu.h"
#include "traits.h"

#include <assert.h>
#include <stdlib.h>

// The NES-001 Motherboard including the CPU/Audio Generator, PPU,
// RAM, VRAM, and memory-mapped banks for Cartridge RAM/ROM and
// Controller Input.

struct nes_console {
    struct mos6502 cpu;     // CPU Core of RP2A03 Chip
    uint8_t ram[RAM_SIZE],  // CPU Internal RAM
            cart[ROM_SIZE]; // TODO: Cartridge ROM to be replaced
                            // eventually with cartridge + mapper
};

static void load_prg(nes *self, size_t sz, const uint8_t prg[restrict sz])
{
    // TODO: stick test programs at 0x8000 for now
    assert(sz <= ROM_SIZE);

    for (size_t i = 0; i < sz; ++i) {
        self->cart[(i + CpuCartMinAddr) & CpuCartAddrMask] = prg[i];
    }
    wrtoba(CpuCartMinAddr, self->cart + (ResetVector & CpuCartAddrMask));
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

void nes_powerup(nes *self, size_t sz, const uint8_t prg[restrict sz])
{
    assert(self != NULL);

    cpu_powerup(&self->cpu);
    load_prg(self, sz, prg);
    cpu_reset(&self->cpu);
}

int nes_cycle(nes *self)
{
    assert(self != NULL);

    self->cpu.signal.rdy = true;
    const int cycles = cpu_clock(&self->cpu, 1);
    self->cpu.signal.rdy = false;
    return cycles;
}

int nes_step(nes *self)
{
    assert(self != NULL);

    int cycles = 0;
    self->cpu.signal.rdy = true;
    do {
        cycles += cpu_clock(&self->cpu, 1);
    } while (!self->cpu.signal.sync);
    self->cpu.signal.rdy = false;
    return cycles;
}

int nes_clock(nes *self)
{
    assert(self != NULL);

    return 0;
}

void nes_snapshot(nes *self, struct console_state *snapshot)
{
    assert(self != NULL);
    if (!snapshot) return;

    cpu_snapshot(&self->cpu, snapshot);
    snapshot->ram = self->ram;
    snapshot->rom = self->cart;
}
