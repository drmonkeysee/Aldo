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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// The NES-001 Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct nes_console {
    cart *cart;                 // Game Cartridge; Non-owning Pointer
    debugctx *dbg;              // Debugger Context; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    struct mos6502 cpu;         // CPU Core of RP2A03 Chip
    enum csig_excmode mode;     // NES execution mode
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
    debug_cpu_connect(self->dbg, &self->cpu);
}

static void connect_cart(struct nes_console *self, cart *c)
{
    self->cart = c;
    cart_cpu_connect(self->cart, self->cpu.bus, MEMBLOCK_32KB);
    debug_sync_bus(self->dbg);
}

static void disconnect_cart(struct nes_console *self)
{
    // NOTE: debugger may have been attached to a cart-less CPU bus so reset
    // debugger even if there is no existing cart.
    debug_reset(self->dbg);
    if (!self->cart) return;
    cart_cpu_disconnect(self->cart, self->cpu.bus, MEMBLOCK_32KB);
    self->cart = NULL;
}

static void free_cpubus(struct nes_console *self)
{
    debug_cpu_disconnect(self->dbg);
    bus_free(self->cpu.bus);
    self->cpu.bus = NULL;
}

static void set_interrupt(struct nes_console *self, enum csig_interrupt signal,
                          bool value)
{
    switch (signal) {
    case CSGI_IRQ:
        self->cpu.signal.irq = value;
        break;
    case CSGI_NMI:
        self->cpu.signal.nmi = value;
        break;
    case CSGI_RES:
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

    struct console_state snapshot;
    nes_snapshot(self, &snapshot);
    // NOTE: trace the cycle count up to the current instruction so do not
    // count the just-executed instruction fetch cycle.
    trace_line(self->tracelog, clock->total_cycles - 1, &self->cpu, self->dbg,
               &snapshot);
}

//
// Public Interface
//

nes *nes_new(debugctx *dbg, bool bcdsupport, FILE *tracelog)
{
    assert(dbg != NULL);

    struct nes_console *const self = malloc(sizeof *self);
    self->cart = NULL;
    self->dbg = dbg;
    self->tracelog = tracelog;
    // TODO: ditch this option when aldo can emulate more than just NES
    self->cpu.bcd = bcdsupport;
    create_cpubus(self);
    return self;
}

void nes_free(nes *self)
{
    assert(self != NULL);

    disconnect_cart(self);
    free_cpubus(self);
    free(self);
}

void nes_powerup(nes *self, cart *c, bool zeroram)
{
    assert(self != NULL);
    assert(self->cart == NULL);

    if (c) {
        connect_cart(self, c);
    }
    // TODO: for now start in cycle-step mode
    self->mode = CSGM_CYCLE;
    if (zeroram) {
        memset(self->ram, 0, sizeof self->ram / sizeof self->ram[0]);
    }
    cpu_powerup(&self->cpu);
}

void nes_powerdown(nes *self)
{
    assert(self != NULL);

    nes_halt(self);
    disconnect_cart(self);
}

size_t nes_ram_size(nes *self)
{
    assert(self != NULL);

    return sizeof self->ram / sizeof self->ram[0];
}

bool nes_bcd_support(nes *self)
{
    assert(self != NULL);

    return self->cpu.bcd;
}

enum csig_excmode nes_mode(nes *self)
{
    assert(self != NULL);

    return self->mode;
}

void nes_set_mode(nes *self, enum csig_excmode mode)
{
    assert(self != NULL);

    // NOTE: force signed to check < 0 (underlying type may be uint)
    self->mode = (int)mode < 0 ? CSGM_MODECOUNT - 1 : mode % CSGM_MODECOUNT;
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
void nes_interrupt(nes *self, enum csig_interrupt signal)
{
    assert(self != NULL);

    set_interrupt(self, signal, false);
}

void nes_clear(nes *self, enum csig_interrupt signal)
{
    assert(self != NULL);

    set_interrupt(self, signal, true);
}

void nes_cycle(nes *self, struct cycleclock *clock)
{
    assert(self != NULL);
    assert(clock != NULL);

    while (self->cpu.signal.rdy && clock->budget > 0) {
        const int cycles = cpu_cycle(&self->cpu);
        clock->budget -= cycles;
        clock->total_cycles += (uint64_t)cycles;
        instruction_trace(self, clock);

        switch (self->mode) {
        default:
            assert(((void)"INVALID EXC MODE", false));
            // NOTE: release build fallthrough to single-cycle
        case CSGM_CYCLE:
            nes_halt(self);
            break;
        case CSGM_STEP:
            self->cpu.signal.rdy = !self->cpu.signal.sync;
            break;
        case CSGM_RUN:
            break;
        }
        debug_check(self->dbg, clock);
    }
}

void nes_snapshot(nes *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    cpu_snapshot(&self->cpu, snapshot);
    debug_snapshot(self->dbg, snapshot);
    snapshot->mem.ram = self->ram;
    snapshot->mem.prglength = bus_dma(self->cpu.bus,
                                      snapshot->datapath.current_instruction,
                                      sizeof snapshot->mem.currprg
                                        / sizeof snapshot->mem.currprg[0],
                                      snapshot->mem.currprg);
    bus_dma(self->cpu.bus, CPU_VECTOR_NMI,
            sizeof snapshot->mem.vectors / sizeof snapshot->mem.vectors[0],
            snapshot->mem.vectors);
}

void nes_dumpram(nes *self, FILE *f)
{
    assert(self != NULL);
    assert(f != NULL);

    fwrite(self->ram, sizeof self->ram[0],
           sizeof self->ram / sizeof self->ram[0], f);
}
