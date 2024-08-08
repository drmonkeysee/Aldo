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

#define memsz(mem) (sizeof (mem) / sizeof (mem)[0])

static const int PpuRatio = 3;

// The NES-001 NTSC Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct nes001 {
    cart *cart;                 // Game Cartridge; Non-owning Pointer
    debugger *dbg;              // Debugger Context; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    struct mos6502 cpu;         // CPU Core of RP2A03 Chip
    struct rp2c02 ppu;          // RP2C02 PPU
    enum csig_excmode mode;     // NES execution mode
    struct {
        bool
            irq: 1,             // IRQ Probe
            nmi: 1,             // NMI Probe
            rst: 1;             // RESET Probe
    } probe;                    // Interrupt Input Probes (active high)
    uint8_t ram[MEMBLOCK_2KB],  // CPU Internal RAM
            vram[MEMBLOCK_2KB]; // PPU Internal RAM
};

static void ram_load(uint8_t *restrict d, const uint8_t *restrict mem,
                     uint16_t addr)
{
    *d = mem[addr & ADDRMASK_2KB];
}

static void ram_store(uint8_t *mem, uint16_t addr, uint8_t d)
{
    mem[addr & ADDRMASK_2KB] = d;
}

static bool ram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < MEMBLOCK_8KB);

    ram_load(d, ctx, addr);
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < MEMBLOCK_8KB);

    ram_store(ctx, addr, d);
    return true;
}

static size_t ram_dma(const void *restrict ctx, uint16_t addr, size_t count,
                      uint8_t dest[restrict count])
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < MEMBLOCK_8KB);

    // NOTE: only 2KB of actual mem to copy
    return bytecopy_bank(ctx, BITWIDTH_2KB, addr, count, dest);
}

static bool vram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$2000-$3FFF]
    // NOTE: palette reads still hit the VRAM bus and affect internal PPU
    // buffers, so the full 8KB range is valid input.
    assert(MEMBLOCK_8KB <= addr && addr < MEMBLOCK_16KB);

    ram_load(d, ctx, addr);
    return true;
}

static bool vram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$2000-$3EFF]
    // NOTE: writes to palette RAM should never hit the video bus
    assert(MEMBLOCK_8KB <= addr && addr < PaletteStartAddr);

    ram_store(ctx, addr, d);
    return true;
}

static size_t vram_dma(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // NOTE: addr=[$2000-$3FFF]
    // NOTE: full 8KB range is valid input, see vram_read for reasons
    assert(MEMBLOCK_8KB <= addr && addr < MEMBLOCK_16KB);

    // NOTE: only 2KB of actual mem to copy
    return bytecopy_bank(ctx, BITWIDTH_2KB, addr, count, dest);
}

static void create_mbus(struct nes001 *self)
{
    // TODO: partitions so far:
    // 16-bit Address Space = 64KB
    // * $0000 - $1FFF: 2KB RAM mirrored to 8KB
    // * $2000 - $3FFF: 8 PPU registers mirrored to 8KB
    // * $4000 - $7FFF: unmapped
    // * $8000 - $FFFF: 32KB Cart
    self->cpu.mbus = bus_new(BITWIDTH_64KB, 4, MEMBLOCK_8KB, MEMBLOCK_16KB,
                             MEMBLOCK_32KB);
    bool r = bus_set(self->cpu.mbus, 0, (struct busdevice){
        ram_read,
        ram_write,
        ram_dma,
        self->ram,
    });
    assert(r);
}

static void create_vbus(struct nes001 *self)
{
    // TODO: partitions so far:
    // 14-bit Address Space = 16KB
    // * $0000 - $1FFF: unmapped
    // * $2000 - $3FFF: 2KB RAM mirrored to 8KB, nametable-mirroring uses 4KB
    //                  of address space; never writes to $3F00 - $3FFF
    // * $3F00 - $3FFF: 32B Palette RAM mirrored to 256B; internal to the PPU
    //                  and thus not on the video bus, but reads do leak
    //                  through to the underlying VRAM.
    self->ppu.vbus = bus_new(BITWIDTH_16KB, 2, MEMBLOCK_8KB);
    bool r = bus_set(self->ppu.vbus, MEMBLOCK_8KB, (struct busdevice){
        vram_read,
        vram_write,
        vram_dma,
        self->vram,
    });
    assert(r);
}

static void connect_cart(struct nes001 *self, cart *c)
{
    self->cart = c;
    // TODO: binding to $8000 is too simple; WRAM needs at least $6000, and the
    // CPU memory map defines start of cart mapping at $4020; the most complex
    // mappers need access to entire 64KB address space in order to snoop on
    // all CPU activity. Similar rules hold for PPU.
    bool r = cart_mbus_connect(self->cart, self->cpu.mbus, MEMBLOCK_32KB);
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

static void setup(struct nes001 *self)
{
    create_mbus(self);
    create_vbus(self);
    ppu_connect(&self->ppu, self->cpu.mbus);
    debug_cpu_connect(self->dbg, &self->cpu);
}

static void teardown(struct nes001 *self)
{
    disconnect_cart(self);
    debug_cpu_disconnect(self->dbg);
    bus_free(self->ppu.vbus);
    bus_free(self->cpu.mbus);
}

static void set_ppu_pins(struct nes001 *self)
{
    // NOTE: interrupt lines are active low
    self->ppu.signal.rst = !self->probe.rst;
    // NOTE: pull PPU's CPU R/W signal back up if CPU is no longer pulling it
    // low (pulled low by PPU register writes).
    self->ppu.signal.rw |= self->cpu.signal.rw;
}

static void set_cpu_pins(struct nes001 *self)
{
    // NOTE: interrupt lines are active low
    self->cpu.signal.irq = !self->probe.irq;
    self->cpu.signal.nmi = !self->probe.nmi && self->ppu.signal.intr;
    self->cpu.signal.rst = !self->probe.rst;
}

// NOTE: trace the just-fetched instruction
static void instruction_trace(struct nes001 *self,
                              const struct cycleclock *clock, int adjustment)
{
    if (!self->tracelog || !self->cpu.signal.sync) return;

    struct snapshot snp;
    nes_snapshot(self, &snp);
    // NOTE: trace the cycle/pixel count up to the current instruction so
    // do NOT count the just-executed instruction fetch cycle.
    trace_line(self->tracelog, clock->cycles + (uint64_t)adjustment,
               ppu_trace(&self->ppu, adjustment * PpuRatio), &self->cpu,
               self->dbg, &snp);
}

static bool clock_ppu(struct nes001 *self, struct cycleclock *clock)
{
    clock->frames += (uint64_t)ppu_cycle(&self->ppu);
    --clock->budget;
    set_ppu_pins(self);
    // TODO: ppu debug hook goes here
    if (++clock->subcycle < PpuRatio) {
        if (self->mode == CSGM_SUBCYCLE) {
            nes_ready(self, false);
        }
        return false;
    }
    return true;
}

static void clock_cpu(struct nes001 *self, struct cycleclock *clock)
{
    int cycles = cpu_cycle(&self->cpu);
    set_cpu_pins(self);
    clock->subcycle = 0;
    clock->cycles += (uint64_t)cycles;
    instruction_trace(self, clock, -cycles);

    switch (self->mode) {
    // NOTE: both cases are possible on cycle-boundary
    case CSGM_SUBCYCLE:
    case CSGM_CYCLE:
        nes_ready(self, false);
        break;
    case CSGM_STEP:
        nes_ready(self, !self->cpu.signal.sync);
        break;
    case CSGM_RUN:
    default:
        break;
    }
    debug_check(self->dbg, clock);
}

//
// MARK: - Public Interface
//

nes *nes_new(debugger *dbg, bool bcdsupport, FILE *tracelog)
{
    assert(dbg != NULL);

    struct nes001 *self = malloc(sizeof *self);
    self->cart = NULL;
    self->dbg = dbg;
    self->tracelog = tracelog;
    // TODO: ditch this option when aldo can emulate more than just NES
    self->cpu.bcd = bcdsupport;
    self->probe.irq = self->probe.nmi = self->probe.rst = false;
    setup(self);
    return self;
}

void nes_free(nes *self)
{
    assert(self != NULL);

    teardown(self);
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
        memset(self->ram, 0, memsz(self->ram));
        memset(self->vram, 0, memsz(self->vram));
        ppu_zeroram(&self->ppu);
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

    return memsz(self->ram);
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
    self->mode = (int)mode < 0 ? CSGM_COUNT - 1 : mode % CSGM_COUNT;
}

void nes_ready(nes *self, bool ready)
{
    assert(self != NULL);

    self->cpu.signal.rdy = ready;
}

bool nes_probe(nes *self, enum csig_interrupt signal)
{
    assert(self != NULL);

    switch (signal) {
    case CSGI_IRQ:
        return self->probe.irq;
    case CSGI_NMI:
        return self->probe.nmi;
    case CSGI_RST:
        return self->probe.rst;
    default:
        assert(((void)"INVALID NES PROBE", false));
        return false;
    }
}

void nes_set_probe(nes *self, enum csig_interrupt signal, bool active)
{
    assert(self != NULL);

    switch (signal) {
    case CSGI_IRQ:
        self->probe.irq = active;
        break;
    case CSGI_NMI:
        self->probe.nmi = active;
        break;
    case CSGI_RST:
        self->probe.rst = active;
        break;
    default:
        assert(((void)"INVALID NES PROBE", false));
        break;
    }
}

void nes_clock(nes *self, struct cycleclock *clock)
{
    assert(self != NULL);
    assert(clock != NULL);

    while (self->cpu.signal.rdy && clock->budget > 0) {
        if (!clock_ppu(self, clock)) continue;
        clock_cpu(self, clock);
    }
}

int nes_cycle_factor(void)
{
    return PpuRatio;
}

int nes_frame_factor(void)
{
    return DotsPerFrame;
}

void nes_snapshot(nes *self, struct snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);

    cpu_snapshot(&self->cpu, snp);
    ppu_snapshot(&self->ppu, snp);
    debug_snapshot(self->dbg, snp);
    snp->mem.ram = self->ram;
    snp->mem.vram = self->vram;
    snp->mem.prglength = bus_dma(self->cpu.mbus,
                                 snp->datapath.current_instruction,
                                 memsz(snp->mem.currprg), snp->mem.currprg);
    bus_dma(self->cpu.mbus, CPU_VECTOR_NMI, memsz(snp->mem.vectors),
            snp->mem.vectors);
}

void nes_dumpram(nes *self, FILE *fs[static 3])
{
    assert(self != NULL);
    assert(fs != NULL);

    FILE *f;
    if ((f = fs[0])) {
        fwrite(self->ram, sizeof self->ram[0], memsz(self->ram), f);
    }
    if ((f = fs[1])) {
        fwrite(self->vram, sizeof self->vram[0], memsz(self->vram), f);
    }
    if ((f = fs[2])) {
        ppu_dumpram(&self->ppu, f);
    }
}
