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
#include <string.h>

// The NES-001 Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct nes_console {
    struct mos6502 cpu;         // CPU Core of RP2A03 Chip
    cart *cart;                 // Game Cartridge; Non-owning Pointer
    enum nexcmode mode;         // NES execution mode
    uint8_t ram[NES_RAM_SIZE];  // CPU Internal RAM
};

static bool ram_read(const void *restrict ctx, uint16_t addr,
                     uint8_t *restrict d)
{
    *d = ((const uint8_t *)ctx)[addr & CpuRamAddrMask];
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    ((uint8_t *)ctx)[addr & CpuRamAddrMask] = d;
    return true;
}

static size_t ram_dma(const void *restrict ctx, uint16_t addr,
                      uint8_t *restrict dest, size_t count)
{
    uint16_t bankstart = addr & CpuRamAddrMask;
    const size_t ramspace = (CpuRamMaxAddr + 1) - addr,
                 maxcount = count > ramspace ? ramspace : count,
                 bankcount = NES_RAM_SIZE - bankstart;
    const uint8_t *ram = ctx;
    size_t bytescopy = maxcount > bankcount ? bankcount : maxcount;
    ptrdiff_t bytesleft = maxcount;
    // NOTE: 2KB bank in 8KB space means DMA needs to:
    // 1) start copy from some offset within the 2KB bank (or a bank mirror)
    // 2) wraparound if DMA crosses bank boundary within address space
    // 3) stop copy at end of address window
    do {
        memcpy(dest, ram + bankstart, bytescopy * sizeof *dest);
        // NOTE: first memcpy chunk may be offset from start of bank; any
        // additional chunks start at 0 due to mirroring-wraparound.
        bankstart = 0;
        bytesleft -= bytescopy;
        dest += bytescopy;
        bytescopy = bytesleft > (ptrdiff_t)NES_RAM_SIZE
                    ? NES_RAM_SIZE
                    : bytesleft;
    } while (bytesleft > 0);
    // NOTE: if we went negative our math is wrong
    assert(bytesleft == 0);
    return maxcount;
}

static void create_cpubus(struct nes_console *self)
{
    // TODO: 3 partitions only for now, 8KB ram, 32KB rom, nothing in between
    self->cpu.bus = bus_new(16, 3, CpuRamMaxAddr + 1, CpuRomMinAddr);
    bus_set(self->cpu.bus, 0,
            (struct busdevice){ram_read, ram_write, ram_dma, self->ram});
    cart_cpu_connect(self->cart, self->cpu.bus, CpuRomMinAddr);
}

static void free_cpubus(struct nes_console *self)
{
    cart_cpu_disconnect(self->cart, self->cpu.bus, CpuRomMinAddr);
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
    snapshot->mem.ram = snapshot->ram = self->ram;
    snapshot->mem.prglength = bus_dma(self->cpu.bus,
                                      snapshot->datapath.current_instruction,
                                      snapshot->mem.prgview,
                                      sizeof snapshot->mem.prgview
                                      / sizeof snapshot->mem.prgview[0]);
    bus_dma(self->cpu.bus, NmiVector, snapshot->mem.vectors,
            sizeof snapshot->mem.vectors / sizeof snapshot->mem.vectors[0]);
}
