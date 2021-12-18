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
#include "trace.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct resdecorator {
    struct busdevice inner;
    uint16_t vector;
};

// The NES-001 Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct nes_console {
    cart *cart;                 // Game Cartridge; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    struct resdecorator *dec;   // Device Decorator for RESET Vector Override
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

static bool resetaddr_read(const void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    struct resdecorator *const dec = (struct resdecorator *)ctx;
    if (!dec->inner.read) return false;

    if (CPU_VECTOR_RES <= addr && addr < CPU_VECTOR_IRQ) {
        uint8_t vector[2];
        wrtoba(dec->vector, vector);
        *d = vector[addr - CPU_VECTOR_RES];
        return true;
    }
    return dec->inner.read(dec->inner.ctx, addr, d);
}

static bool resetaddr_write(void *ctx, uint16_t addr, uint8_t d)
{
    struct resdecorator *const dec = (struct resdecorator *)ctx;
    return dec->inner.write
            ? dec->inner.write(dec->inner.ctx, addr, d)
            : false;
}

static size_t resetaddr_dma(const void *restrict ctx, uint16_t addr,
                            size_t count, uint8_t dest[restrict count])
{
    struct resdecorator *const dec = (struct resdecorator *)ctx;
    return dec->inner.dma
            ? dec->inner.dma(dec->inner.ctx, addr, count, dest)
            : 0;
}

static void create_cpubus(struct nes_console *self, int resetoverride)
{
    // TODO: 3 partitions only for now, 8KB ram, 32KB rom, nothing in between
    self->cpu.bus = bus_new(16, 3, MEMBLOCK_8KB, MEMBLOCK_32KB);
    bus_set(self->cpu.bus, 0,
            (struct busdevice){ram_read, ram_write, ram_dma, self->ram});

    const uint16_t cart_busaddr = MEMBLOCK_32KB;
    cart_cpu_connect(self->cart, self->cpu.bus, cart_busaddr);

    if (resetoverride >= 0) {
        self->dec = malloc(sizeof *self->dec);
        self->dec->vector = resetoverride;
        struct busdevice resetaddr_device = {
            resetaddr_read, resetaddr_write, resetaddr_dma, self->dec,
        };
        bus_swap(self->cpu.bus, cart_busaddr, resetaddr_device,
                 &self->dec->inner);
    }
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

// NOTE: trace the just-fetched instruction
static void instruction_trace(struct nes_console *self,
                              const struct cycleclock *clock)
{
    if (!self->cpu.signal.sync) return;

    // NOTE: trace the cycle count up to the current instruction so do not
    // count the just-executed instruction fetch cycle.
    trace_line(self->tracelog, clock->total_cycles - 1, &self->cpu);
}

//
// Public Interface
//

nes *nes_new(cart *c, FILE *tracelog, int resetoverride)
{
    assert(c != NULL);

    struct nes_console *const self = malloc(sizeof *self);
    self->cart = c;
    self->tracelog = tracelog;
    self->dec = NULL;
    create_cpubus(self, resetoverride);
    return self;
}

void nes_free(nes *self)
{
    assert(self != NULL);

    free_cpubus(self);
    free(self->dec);
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

void nes_cycle(nes *self, struct cycleclock *clock)
{
    assert(self != NULL);
    assert(clock != NULL);

    int cycles = 0;
    while (self->cpu.signal.rdy && cycles < clock->budget) {
        cycles += cpu_cycle(&self->cpu);
        clock->budget -= cycles;
        clock->total_cycles += cycles;
        instruction_trace(self, clock);

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
    snapshot->mem.resvector_override = self->dec ? self->dec->vector : -1;
    bus_dma(self->cpu.bus, CPU_VECTOR_NMI,
            sizeof snapshot->mem.vectors / sizeof snapshot->mem.vectors[0],
            snapshot->mem.vectors);
}
