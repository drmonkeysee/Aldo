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
#include "ppu.h"
#include "trace.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// The NES-001 NTSC Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct nes001 {
    cart *cart;                 // Game Cartridge; Non-owning Pointer
    debugger *dbg;              // Debugger Context; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    struct mos6502 cpu;         // CPU Core of RP2A03 Chip
    struct rp2c02 ppu;          // RP2C02 PPU
    enum csig_excmode mode;     // NES execution mode
    uint8_t ram[MEMBLOCK_2KB],  // CPU Internal RAM
            vram[MEMBLOCK_2KB]; // PPU Internal RAM
};

static bool ram_read(const void *restrict ctx, uint16_t addr,
                     uint8_t *restrict d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < MEMBLOCK_8KB);

    *d = ((const uint8_t *)ctx)[addr & ADDRMASK_2KB];
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < MEMBLOCK_8KB);

    ((uint8_t *)ctx)[addr & ADDRMASK_2KB] = d;
    return true;
}

static size_t ram_dma(const void *restrict ctx, uint16_t addr, size_t count,
                      uint8_t dest[restrict count])
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < MEMBLOCK_8KB);

    return bytecopy_bankmirrored(ctx, BITWIDTH_2KB, addr, BITWIDTH_8KB, count,
                                 dest);
}

static void create_mbus(struct nes001 *self)
{
    // TODO: partitions so far:
    // * $0000 - $1FFF: 8KB RAM
    // * $2000 - $3FFF: 8KB PPU registers
    // * $4000 - $7FFF: unmapped
    // * $8000 - $FFFF: 32KB Cart
    self->cpu.mbus = bus_new(BITWIDTH_64KB, 4, MEMBLOCK_8KB, MEMBLOCK_16KB,
                             MEMBLOCK_32KB);
    const bool r = bus_set(self->cpu.mbus, 0, (struct busdevice){
        ram_read,
        ram_write,
        ram_dma,
        self->ram,
    });
    assert(r);
    debug_cpu_connect(self->dbg, &self->cpu);
}

static void create_vbus(struct nes001 *self)
{
    // TODO: partitions so far:
    // * $0000 - $3FFF: unmapped
    self->ppu.vbus = bus_new(BITWIDTH_16KB, 1);
    // TODO: add vram device
}

static void connect_ppu(struct nes001 *self)
{
    ppu_connect(&self->ppu, self->vram, self->cpu.mbus);
}

static void connect_cart(struct nes001 *self, cart *c)
{
    self->cart = c;
    // TODO: binding to $8000 is too simple; WRAM needs at least $6000, and the
    // CPU memory map defines start of cart mapping at $4020; the most complex
    // mappers need access to entire 64KB address space in order to snoop on
    // all CPU activity. Similar rules hold for PPU.
    const bool r = cart_mbus_connect(self->cart, self->cpu.mbus,
                                     MEMBLOCK_32KB);
    assert(r);
    debug_sync_bus(self->dbg);
}

static void disconnect_cart(struct nes001 *self)
{
    // NOTE: debugger may have been attached to a cart-less CPU bus so reset
    // debugger even if there is no existing cart.
    debug_reset(self->dbg);
    if (!self->cart) return;
    cart_mbus_disconnect(self->cart, self->cpu.mbus, MEMBLOCK_32KB);
    self->cart = NULL;
}

static void free_vbus(struct nes001 *self)
{
    bus_free(self->ppu.vbus);
    self->ppu.vbus = NULL;
}

static void free_mbus(struct nes001 *self)
{
    debug_cpu_disconnect(self->dbg);
    bus_free(self->cpu.mbus);
    self->cpu.mbus = NULL;
}

static void set_interrupt(struct nes001 *self, enum csig_interrupt signal,
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
        self->ppu.signal.res = self->cpu.signal.res = value;
        break;
    default:
        assert(((void)"INVALID NES INTERRUPT", false));
        break;
    }
}

// NOTE: trace the just-fetched instruction
static void instruction_trace(struct nes001 *self,
                              const struct cycleclock *clock)
{
    if (!self->tracelog || !self->cpu.signal.sync) return;

    struct snapshot snp;
    nes_snapshot(self, &snp);
    // NOTE: trace the cycle/pixel count up to the current instruction so
    // do NOT count the just-executed instruction fetch cycle.
    trace_line(self->tracelog, clock->total_cycles - 1,
               ppu_pixel_trace(&self->ppu, -1), &self->cpu, self->dbg, &snp);
}

//
// Public Interface
//

nes *nes_new(debugger *dbg, bool bcdsupport, FILE *tracelog)
{
    assert(dbg != NULL);

    struct nes001 *const self = malloc(sizeof *self);
    self->cart = NULL;
    self->dbg = dbg;
    self->tracelog = tracelog;
    // TODO: ditch this option when aldo can emulate more than just NES
    self->cpu.bcd = bcdsupport;
    create_mbus(self);
    create_vbus(self);
    connect_ppu(self);
    return self;
}

void nes_free(nes *self)
{
    assert(self != NULL);

    disconnect_cart(self);
    free_vbus(self);
    free_mbus(self);
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
    ppu_powerup(&self->ppu);
}

void nes_powerdown(nes *self)
{
    assert(self != NULL);

    nes_ready(self, false);
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

void nes_ready(nes *self, bool ready)
{
    assert(self != NULL);

    self->cpu.signal.rdy = ready;
}

void nes_interrupt(nes *self, enum csig_interrupt signal, bool active)
{
    assert(self != NULL);

    // NOTE: interrupt lines are active low
    set_interrupt(self, signal, !active);
}

void nes_cycle(nes *self, struct cycleclock *clock)
{
    assert(self != NULL);
    assert(clock != NULL);

    while (self->cpu.signal.rdy && clock->budget > 0) {
        const int cycles = cpu_cycle(&self->cpu);
        ppu_cycle(&self->ppu, cycles);
        clock->budget -= cycles;
        clock->total_cycles += (uint64_t)cycles;
        instruction_trace(self, clock);

        switch (self->mode) {
        default:
            assert(((void)"INVALID EXC MODE", false));
            // NOTE: release build fallthrough to single-cycle
        case CSGM_CYCLE:
            nes_ready(self, false);
            break;
        case CSGM_STEP:
            nes_ready(self, !self->cpu.signal.sync);
            break;
        case CSGM_RUN:
            break;
        }
        debug_check(self->dbg, clock);
    }
}

void nes_snapshot(nes *self, struct snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);

    cpu_snapshot(&self->cpu, snp);
    ppu_snapshot(&self->ppu, snp);
    debug_snapshot(self->dbg, snp);
    snp->mem.ram = self->ram;
    snp->mem.prglength = bus_dma(self->cpu.mbus,
                                 snp->datapath.current_instruction,
                                 sizeof snp->mem.currprg
                                    / sizeof snp->mem.currprg[0],
                                 snp->mem.currprg);
    bus_dma(self->cpu.mbus, CPU_VECTOR_NMI,
            sizeof snp->mem.vectors / sizeof snp->mem.vectors[0],
            snp->mem.vectors);
}

void nes_dumpram(nes *self, FILE *f)
{
    assert(self != NULL);
    assert(f != NULL);

    fwrite(self->ram, sizeof self->ram[0],
           sizeof self->ram / sizeof self->ram[0], f);
}
