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
    enum nexcmode mode;     // NES execution mode
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

    // TODO: throw random stuff into RAM for testing
    for (size_t i = 0; i < RAM_SIZE; ++i) {
        self->ram[i] = rand() % 0x100;
    }
}

//
// Public Interface
//

nes *nes_new(void)
{
    struct nes_console *const self = malloc(sizeof *self);
    self->cpu.ram = self->ram;
    self->cpu.cart = self->cart;
    return self;
}

void nes_free(nes *self)
{
    free(self);
}

void nes_mode(nes *self, enum nexcmode mode)
{
    assert(self != NULL);

    self->mode = mode < 0 ? NEXC_MODECOUNT - 1 : mode % NEXC_MODECOUNT;
}

void nes_powerup(nes *self, size_t sz, const uint8_t prg[restrict sz])
{
    assert(self != NULL);

    cpu_powerup(&self->cpu);
    load_prg(self, sz, prg);
    cpu_reset(&self->cpu);
}

void nes_ready(nes *self)
{
    assert(self != NULL);

    self->cpu.signal.rdy = true;
}

void nes_halt(nes *self)
{
    assert(self != NULL);

    self->cpu.signal.rdy = false;
}

int nes_cycle(nes *self, int cpubudget)
{
    assert(self != NULL);

    int cycles = 0;
    while (self->cpu.signal.rdy && cycles < cpubudget) {
        cycles += cpu_cycle(&self->cpu);
        switch (self->mode) {
        case NEXC_CYCLE:
            self->cpu.signal.rdy = false;
            break;
        case NEXC_STEP:
            self->cpu.signal.rdy = !self->cpu.signal.sync;
            break;
        case NEXC_RUN:
            break;
        default:
            assert(((void)"INVALID EXC MODE", false));
        }
    }
    return cycles;
}

int nes_clock(nes *self)
{
    assert(self != NULL);

    // TODO: pretend 1000 cycles is one frame for now
    return nes_cycle(self, 1000);
}

void nes_snapshot(nes *self, struct console_state *snapshot)
{
    assert(self != NULL);
    if (!snapshot) return;

    cpu_snapshot(&self->cpu, snapshot);
    snapshot->mode = self->mode;
    snapshot->ram = self->ram;
    snapshot->rom = self->cart;
}
