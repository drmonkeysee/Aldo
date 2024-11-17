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
#include "cycleclock.h"
#include "ppu.h"
#include "snapshot.h"
#include "trace.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define memsz(mem) (sizeof (mem) / sizeof (mem)[0])

static const int PpuRatio = 3;

enum {
    SCREEN_WIDTH = 256,
    SCREEN_HEIGHT = 240,
};

// The NES-001 NTSC Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct aldo_nes001 {
    aldo_cart *cart;            // Game Cartridge; Non-owning Pointer
    aldo_debugger *dbg;         // Debugger Context; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    struct aldo_mos6502 cpu;    // CPU Core of RP2A03 Chip
    struct aldo_rp2c02 ppu;     // RP2C02 PPU
    enum aldo_execmode mode;    // NES execution mode
    struct {
        bool
            irq: 1,                     // IRQ Probe
            nmi: 1,                     // NMI Probe
            rst: 1;                     // RESET Probe
    } probe;                            // Interrupt Input Probes (active high)
    bool tracefailed;                   // Trace log I/O failed during run
    uint8_t ram[ALDO_MEMBLOCK_2KB],     // CPU Internal RAM
            vram[ALDO_MEMBLOCK_2KB],    // PPU Internal RAM
            screen[SCREEN_WIDTH * SCREEN_HEIGHT];   // Video Buffer
};

static void ram_load(uint8_t *restrict d, const uint8_t *restrict mem,
                     uint16_t addr)
{
    *d = mem[addr & ALDO_ADDRMASK_2KB];
}

static void ram_store(uint8_t *mem, uint16_t addr, uint8_t d)
{
    mem[addr & ALDO_ADDRMASK_2KB] = d;
}

static bool ram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    ram_load(d, ctx, addr);
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    ram_store(ctx, addr, d);
    return true;
}

static size_t ram_copy(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    // NOTE: only 2KB of actual mem to copy
    return aldo_bytecopy_bank(ctx, ALDO_BITWIDTH_2KB, addr, count, dest);
}

static bool vram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$2000-$3FFF]
    // NOTE: palette reads still hit the VRAM bus and affect internal PPU
    // buffers, so the full 8KB range is valid input.
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    ram_load(d, ctx, addr);
    return true;
}

static bool vram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$2000-$3EFF]
    // NOTE: writes to palette RAM should never hit the video bus
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < Aldo_PaletteStartAddr);

    ram_store(ctx, addr, d);
    return true;
}

static size_t vram_copy(const void *restrict ctx, uint16_t addr, size_t count,
                        uint8_t dest[restrict count])
{
    // NOTE: addr=[$2000-$3FFF]
    // NOTE: full 8KB range is valid input, see vram_read for reasons
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    // NOTE: only 2KB of actual mem to copy
    return aldo_bytecopy_bank(ctx, ALDO_BITWIDTH_2KB, addr, count, dest);
}

static bool create_mbus(struct aldo_nes001 *self)
{
    // TODO: partitions so far:
    // 16-bit Address Space = 64KB
    // * $0000 - $1FFF: 2KB RAM mirrored to 8KB
    // * $2000 - $3FFF: 8 PPU registers mirrored to 8KB
    // * $4000 - $7FFF: unmapped
    // * $8000 - $FFFF: 32KB Cart
    self->cpu.mbus = aldo_bus_new(ALDO_BITWIDTH_64KB, 4, ALDO_MEMBLOCK_8KB,
                                  ALDO_MEMBLOCK_16KB, ALDO_MEMBLOCK_32KB);
    if (!self->cpu.mbus) return false;

    bool r = aldo_bus_set(self->cpu.mbus, 0, (struct aldo_busdevice){
        ram_read,
        ram_write,
        ram_copy,
        self->ram,
    });
    (void)r, assert(r);
    return true;
}

static bool create_vbus(struct aldo_nes001 *self)
{
    // 14-bit Address Space = 16KB
    // * $0000 - $1FFF: 8KB CHR ROM/RAM, 2 pattern tables mapped to cartridge
    // * $2000 - $3FFF: 2KB RAM mirrored to 8KB, nametable-mirroring uses 4KB
    //                  of address space; never writes to $3F00 - $3FFF
    // * $3F00 - $3FFF: 32B Palette RAM mirrored to 256B; internal to the PPU
    //                  and thus not on the video bus, but reads do leak
    //                  through to the underlying VRAM.
    self->ppu.vbus = aldo_bus_new(ALDO_BITWIDTH_16KB, 2, ALDO_MEMBLOCK_8KB);
    if (!self->ppu.vbus) return false;

    bool r = aldo_bus_set(self->ppu.vbus, ALDO_MEMBLOCK_8KB,
                          (struct aldo_busdevice){
        vram_read,
        vram_write,
        vram_copy,
        self->vram,
    });
    (void)r, assert(r);
    return true;
}

static void connect_cart(struct aldo_nes001 *self, aldo_cart *c)
{
    self->cart = c;
    bool r = aldo_cart_mbus_connect(self->cart, self->cpu.mbus);
    (void)r, assert(r);
    aldo_cart_vbus_connect(self->cart, self->ppu.vbus);
    aldo_debug_sync_bus(self->dbg);
}

static void disconnect_cart(struct aldo_nes001 *self)
{
    // NOTE: debugger may have been attached to a cart-less CPU bus so reset
    // debugger even if there is no existing cart.
    aldo_debug_reset(self->dbg);
    if (!self->cart) return;
    aldo_cart_vbus_disconnect(self->cart, self->ppu.vbus);
    aldo_cart_mbus_disconnect(self->cart, self->cpu.mbus);
    self->cart = NULL;
}

static bool setup(struct aldo_nes001 *self)
{
    if (!create_mbus(self)) return false;
    if (!create_vbus(self)) return false;
    aldo_ppu_connect(&self->ppu, self->cpu.mbus);
    aldo_debug_cpu_connect(self->dbg, &self->cpu);
    return true;
}

static void teardown(struct aldo_nes001 *self)
{
    disconnect_cart(self);
    aldo_debug_cpu_disconnect(self->dbg);
    aldo_bus_free(self->ppu.vbus);
    aldo_bus_free(self->cpu.mbus);
}

static void set_ppu_pins(struct aldo_nes001 *self)
{
    // NOTE: interrupt lines are active low
    self->ppu.signal.rst = !self->probe.rst;
    // NOTE: pull PPU's CPU R/W signal back up if CPU is no longer pulling it
    // low (pulled low by PPU register writes).
    self->ppu.signal.rw |= self->cpu.signal.rw;
}

static void set_cpu_pins(struct aldo_nes001 *self)
{
    // NOTE: interrupt lines are active low
    self->cpu.signal.irq = !self->probe.irq;
    self->cpu.signal.nmi = !self->probe.nmi && self->ppu.signal.intr;
    self->cpu.signal.rst = !self->probe.rst;
}

static void bus_snapshot(const struct aldo_nes001 *self,
                         struct aldo_snapshot *snp)
{
    aldo_cpu_snapshot(&self->cpu, snp);
    aldo_ppu_bus_snapshot(&self->ppu, snp);
    aldo_bus_copy(self->cpu.mbus, ALDO_CPU_VECTOR_NMI, memsz(snp->prg.vectors),
                  snp->prg.vectors);
}

// NOTE: trace the just-fetched instruction
static void instruction_trace(struct aldo_nes001 *self,
                              const struct aldo_clock *clock, int adjustment)
{
    if (!self->tracelog || self->tracefailed || !self->cpu.signal.sync) return;

    struct aldo_snapshot snp = {0};
    bus_snapshot(self, &snp);
    struct aldo_ppu_coord c = aldo_ppu_trace(&self->ppu,
                                             adjustment * PpuRatio);
    // NOTE: trace the cycle/pixel count up to the current instruction so
    // do NOT count the just-executed instruction fetch cycle.
    self->tracefailed = !aldo_trace_line(self->tracelog,
                                         clock->cycles + (uint64_t)adjustment,
                                         c, &self->cpu, self->dbg, &snp);
}

static bool clock_ppu(struct aldo_nes001 *self, struct aldo_clock *clock)
{
    clock->frames += (uint64_t)aldo_ppu_cycle(&self->ppu);
    --clock->budget;
    set_ppu_pins(self);
    // TODO: ppu debug hook goes here
    if (++clock->subcycle < PpuRatio) {
        if (self->mode == ALDO_EXC_SUBCYCLE) {
            aldo_nes_ready(self, false);
        }
        return false;
    }
    return true;
}

static void clock_cpu(struct aldo_nes001 *self, struct aldo_clock *clock)
{
    int cycles = aldo_cpu_cycle(&self->cpu);
    set_cpu_pins(self);
    clock->subcycle = 0;
    clock->cycles += (uint64_t)cycles;
    instruction_trace(self, clock, -cycles);

    switch (self->mode) {
    // NOTE: both cases are possible on cycle-boundary
    case ALDO_EXC_SUBCYCLE:
    case ALDO_EXC_CYCLE:
        aldo_nes_ready(self, false);
        break;
    case ALDO_EXC_STEP:
        aldo_nes_ready(self, !self->cpu.signal.sync);
        break;
    case ALDO_EXC_RUN:
    default:
        break;
    }
    aldo_debug_check(self->dbg, clock);
}

//
// MARK: - Public Interface
//

aldo_nes *aldo_nes_new(aldo_debugger *dbg, bool bcdsupport, FILE *tracelog)
{
    assert(dbg != NULL);

    struct aldo_nes001 *self = malloc(sizeof *self);
    if (!self) return self;

    self->cart = NULL;
    self->dbg = dbg;
    self->tracelog = tracelog;
    self->tracefailed = false;
    // TODO: ditch this option when aldo can emulate more than just NES
    self->cpu.bcd = bcdsupport;
    self->probe.irq = self->probe.nmi = self->probe.rst = false;
    if (!setup(self)) {
        aldo_nes_free(self);
        return NULL;
    }
    return self;
}

void aldo_nes_free(aldo_nes *self)
{
    assert(self != NULL);

    teardown(self);
    free(self);
}

void aldo_nes_powerup(aldo_nes *self, aldo_cart *c, bool zeroram)
{
    assert(self != NULL);
    assert(self->cart == NULL);

    if (c) {
        connect_cart(self, c);
    }
    // TODO: for now start in cycle-step mode
    self->mode = ALDO_EXC_CYCLE;
    if (zeroram) {
        memset(self->ram, 0, memsz(self->ram));
        memset(self->vram, 0, memsz(self->vram));
        aldo_ppu_zeroram(&self->ppu);
    }
    aldo_cpu_powerup(&self->cpu);
    aldo_ppu_powerup(&self->ppu);
}

void aldo_nes_powerdown(aldo_nes *self)
{
    assert(self != NULL);

    aldo_nes_ready(self, false);
    disconnect_cart(self);
}

int aldo_nes_max_tcpu(void)
{
    return Aldo_MaxTCycle;
}

size_t aldo_nes_ram_size(aldo_nes *self)
{
    assert(self != NULL);

    return memsz(self->ram);
}

void aldo_nes_screen_size(int *width, int *height)
{
    assert(width != NULL);
    assert(height != NULL);

    *width = SCREEN_WIDTH;
    *height = SCREEN_HEIGHT;
}

bool aldo_nes_bcd_support(aldo_nes *self)
{
    assert(self != NULL);

    return self->cpu.bcd;
}

bool aldo_nes_tracefailed(aldo_nes *self)
{
    assert(self != NULL);

    return self->tracefailed;
}

enum aldo_execmode aldo_nes_mode(aldo_nes *self)
{
    assert(self != NULL);

    return self->mode;
}

void aldo_nes_set_mode(aldo_nes *self, enum aldo_execmode mode)
{
    assert(self != NULL);

    // NOTE: force signed to check < 0 (underlying type may be uint)
    self->mode = (int)mode < 0 ? ALDO_EXC_COUNT - 1 : mode % ALDO_EXC_COUNT;
}

void aldo_nes_ready(aldo_nes *self, bool ready)
{
    assert(self != NULL);

    self->cpu.signal.rdy = ready;
}

bool aldo_nes_probe(aldo_nes *self, enum aldo_interrupt signal)
{
    assert(self != NULL);

    switch (signal) {
    case ALDO_INT_IRQ:
        return self->probe.irq;
    case ALDO_INT_NMI:
        return self->probe.nmi;
    case ALDO_INT_RST:
        return self->probe.rst;
    default:
        assert(((void)"INVALID NES PROBE", false));
        return false;
    }
}

void aldo_nes_set_probe(aldo_nes *self, enum aldo_interrupt signal,
                        bool active)
{
    assert(self != NULL);

    switch (signal) {
    case ALDO_INT_IRQ:
        self->probe.irq = active;
        break;
    case ALDO_INT_NMI:
        self->probe.nmi = active;
        break;
    case ALDO_INT_RST:
        self->probe.rst = active;
        break;
    default:
        assert(((void)"INVALID NES PROBE", false));
        break;
    }
}

void aldo_nes_clock(aldo_nes *self, struct aldo_clock *clock)
{
    assert(self != NULL);
    assert(clock != NULL);

    while (self->cpu.signal.rdy && clock->budget > 0) {
        if (!clock_ppu(self, clock)) continue;
        clock_cpu(self, clock);
    }
}

int aldo_nes_cycle_factor(void)
{
    return PpuRatio;
}

int aldo_nes_frame_factor(void)
{
    return Aldo_DotsPerFrame;
}

void aldo_nes_snapshot(aldo_nes *self, struct aldo_snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);
    assert(snp->prg.curr != NULL);

    bus_snapshot(self, snp);
    aldo_ppu_vid_snapshot(&self->ppu, snp);
    if (self->cart) {
        aldo_cart_snapshot(self->cart, snp);
    }
    snp->mem.ram = self->ram;
    snp->mem.vram = self->vram;
    snp->prg.curr->length =
        aldo_bus_copy(self->cpu.mbus, snp->cpu.datapath.current_instruction,
                      memsz(snp->prg.curr->pc), snp->prg.curr->pc);
}

void aldo_nes_dumpram(aldo_nes *self, FILE *fs[static 3], bool errs[static 3])
{
    assert(self != NULL);
    assert(fs != NULL);
    assert(errs != NULL);

    FILE *f;
    size_t witems, wcount;
    if ((f = fs[0])) {
        witems = memsz(self->ram);
        wcount = fwrite(self->ram, sizeof self->ram[0], witems, f);
        errs[0] = wcount < witems;
    }
    if ((f = fs[1])) {
        witems = memsz(self->vram);
        wcount = fwrite(self->vram, sizeof self->vram[0], witems, f);
        errs[1] = wcount < witems;
    }
    if ((f = fs[2])) {
        errs[2] = !aldo_ppu_dumpram(&self->ppu, f);
    }
}
