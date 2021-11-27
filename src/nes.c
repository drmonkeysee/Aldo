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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// The NES-001 Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct nes_console {
    cart *cart;                 // Game Cartridge; Non-owning Pointer
    struct mos6502 cpu;         // CPU Core of RP2A03 Chip
    enum nexcmode mode;         // NES execution mode
    uint8_t ram[MEMBLOCK_2KB];  // CPU Internal RAM
};

static bool ram_read(const void *restrict ctx, uint16_t addr,
                     uint8_t *restrict d)
{
    *d = ((const uint8_t *)ctx)[addr & ADDRMASK_2KB];
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    ((uint8_t *)ctx)[addr & ADDRMASK_2KB] = d;
    return true;
}

static size_t ram_dma(const void *restrict ctx, uint16_t addr, size_t count,
                      uint8_t dest[restrict count])
{
    return bytecopy_bankmirrored(ctx, BITWIDTH_2KB, addr, BITWIDTH_8KB, count,
                                 dest);
}

static void create_cpubus(struct nes_console *self)
{
    // TODO: 3 partitions only for now, 8KB ram, 32KB rom, nothing in between
    self->cpu.bus = bus_new(16, 3, MEMBLOCK_8KB, MEMBLOCK_32KB);
    bus_set(self->cpu.bus, 0,
            (struct busdevice){ram_read, ram_write, ram_dma, self->ram});
    cart_cpu_connect(self->cart, self->cpu.bus, MEMBLOCK_32KB);
}

static void free_cpubus(struct nes_console *self)
{
    cart_cpu_disconnect(self->cart, self->cpu.bus, MEMBLOCK_32KB);
    bus_free(self->cpu.bus);
    self->cpu.bus = NULL;
}

static void set_interrupt(struct nes_console *self, enum nes_interrupt signal,
                          bool value)
{
    switch (signal) {
    case NESI_IRQ:
        self->cpu.signal.irq = value;
        break;
    case NESI_NMI:
        self->cpu.signal.nmi = value;
        break;
    case NESI_RES:
        self->cpu.signal.res = value;
        break;
    default:
        assert(((void)"INVALID NES INTERRUPT", false));
        break;
    }
}

//
// Public Interface
//

nes *nes_new(cart *c)
{
    assert(c != NULL);

    struct nes_console *const self = malloc(sizeof *self);
    self->cart = c;
    create_cpubus(self);
    return self;
}

void nes_free(nes *self)
{
    assert(self != NULL);

    free_cpubus(self);
    free(self);
}

void nes_mode(nes *self, enum nexcmode mode)
{
    assert(self != NULL);

    // NOTE: force signed to check < 0 (underlying type may be uint)
    self->mode = (int)mode < 0 ? NEXC_MODECOUNT - 1 : mode % NEXC_MODECOUNT;
}

void nes_powerup(nes *self)
{
    assert(self != NULL);

    // TODO: throw random stuff into RAM for testing
    for (size_t i = 0; i < sizeof self->ram / sizeof self->ram[0]; ++i) {
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

    set_interrupt(self, signal, false);
}

void nes_clear(nes *self, enum nes_interrupt signal)
{
    assert(self != NULL);

    set_interrupt(self, signal, true);
}

int nes_cycle(nes *self, int cpubudget)
{
    assert(self != NULL);

    int cycles = 0;
    while (self->cpu.signal.rdy && cycles < cpubudget) {
        cycles += cpu_cycle(&self->cpu);
        trace_cpu(&self->cpu, cycles);
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
            break;
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
    snapshot->mem.ram = self->ram;
    snapshot->mem.prglength = bus_dma(self->cpu.bus,
                                      snapshot->datapath.current_instruction,
                                      sizeof snapshot->mem.prgview
                                        / sizeof snapshot->mem.prgview[0],
                                      snapshot->mem.prgview);
    bus_dma(self->cpu.bus, CPU_VECTOR_NMI,
            sizeof snapshot->mem.vectors / sizeof snapshot->mem.vectors[0],
            snapshot->mem.vectors);
}
