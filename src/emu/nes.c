//
//  nes.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "nes.h"

#include "bus.h"
#include "bytes.h"
#include "cpu.h"
#include "traits.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// The NES-001 Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct nes_console {
    struct mos6502 cpu;     // CPU Core of RP2A03 Chip
    cart *cart;             // Game Cartridge
    enum nexcmode mode;     // NES execution mode
    uint8_t ram[RAM_SIZE];  // CPU Internal RAM
};

static bool ram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    *d = ((uint8_t *)ctx)[addr & CpuRamAddrMask];
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    ((uint8_t *)ctx)[addr & CpuRamAddrMask] = d;
    return true;
}

static void create_cpubus(nes *self)
{
    // TODO: 3 partitions only for now, 8KB ram, 32KB rom, nothing in between
    self->cpu.bus = bus_new(16, 3, 0x2000, 0x8000);
    bus_set(self->cpu.bus, 0,
            (struct busdevice){ram_read, ram_write, self->ram});
}

//
// Public Interface
//

nes *nes_new(cart *c)
{
    assert(c != NULL);

    struct nes_console *const self = malloc(sizeof *self);
    create_cpubus(self);
    self->cart = c;
    cart_connectprg(c, self->cpu.bus, 0x8000);
    return self;
}

void nes_free(nes *self)
{
    cart_free(self->cart);
    bus_free(self->cpu.bus);
    free(self);
}

void nes_mode(nes *self, enum nexcmode mode)
{
    assert(self != NULL);

    self->mode = mode < 0 ? NEXC_MODECOUNT - 1 : mode % NEXC_MODECOUNT;
}

void nes_powerup(nes *self)
{
    assert(self != NULL);

    // TODO: vectors hardcoded for now
    uint8_t *const prgbank = cart_prg_bank(self->cart);
    wrtoba(CpuRomMinAddr, prgbank + (ResetVector & CpuRomAddrMask));
    wrtoba(0x8004, prgbank + (IrqVector & CpuRomAddrMask));

    // TODO: throw random stuff into RAM for testing
    for (size_t i = 0; i < RAM_SIZE; ++i) {
        self->ram[i] = rand() % 0x100;
    }

    cpu_powerup(&self->cpu);
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

// NOTE: interrupt lines are active low
void nes_interrupt(nes *self, enum nes_interrupt signal)
{
    assert(self != NULL);

    switch (signal) {
    case NESI_IRQ:
        self->cpu.signal.irq = false;
        break;
    case NESI_NMI:
        self->cpu.signal.nmi = false;
        break;
    case NESI_RES:
        self->cpu.signal.res = false;
        break;
    default:
        assert(((void)"INVALID NES INTERRUPT", false));
    }
}

void nes_clear(nes *self, enum nes_interrupt signal)
{
    assert(self != NULL);

    switch (signal) {
    case NESI_IRQ:
        self->cpu.signal.irq = true;
        break;
    case NESI_NMI:
        self->cpu.signal.nmi = true;
        break;
    case NESI_RES:
        self->cpu.signal.res = true;
        break;
    default:
        assert(((void)"INVALID NES INTERRUPT", false));
    }
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
    cart_snapshot(self->cart, snapshot);
    snapshot->mode = self->mode;
    snapshot->ram = self->ram;
}
