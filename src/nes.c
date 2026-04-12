//
//  nes.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "nes.h"

#include "apu.h"
#include "bus.h"
#include "bytes.h"
#include "consoledef.h"
#include "cpu.h"
#include "cycleclock.h"
#include "ppu.h"
#include "snapshot.h"
#include "trace.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

// The NES-001 NTSC Motherboard including the CPU/APU, PPU, RAM, VRAM,
// Cartridge RAM/ROM and Controller Input.
struct aldo_nes001 {
    struct aldo_console_base extends;
    struct aldo_rp2a03 apu;             // RP2A03 Microprocessor
    struct aldo_rp2c02 ppu;             // RP2C02 PPU
    bool vbuf;                          // Current video buffer to fill
    uint8_t ram[ALDO_MEMBLOCK_2KB],     // CPU Internal RAM
            vram[ALDO_MEMBLOCK_2KB],    // PPU Internal RAM
                                        // Double-buffered Video
            vbufs[2][Aldo_NesScreenWidth * Aldo_NesScreenHeight];
};

static void mem_load(uint8_t *restrict d, const uint8_t *restrict mem,
                     uint16_t addr)
{
    *d = mem[addr & ALDO_ADDRMASK_2KB];
}

static void mem_store(uint8_t *mem, uint16_t addr, uint8_t d)
{
    mem[addr & ALDO_ADDRMASK_2KB] = d;
}

static size_t mem_copy(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // only 2KB of actual mem to copy
    return aldo_bytecopy_bank(ctx, ALDO_BITWIDTH_2KB, addr, count, dest);
}

static bool ram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    mem_load(d, ctx, addr);
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    mem_store(ctx, addr, d);
    return true;
}

static size_t ram_copy(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    return mem_copy(ctx, addr, count, dest);
}

static bool vram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // addr=[$2000-$3FFF]
    // Palette reads still hit the VRAM bus and affect internal PPU
    // buffers, so the full 8KB range is valid input.
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    mem_load(d, ctx, addr);
    return true;
}

static bool vram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // addr=[$2000-$3EFF]
    // writes to palette RAM should never hit the video bus
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < Aldo_PaletteStartAddr);

    mem_store(ctx, addr, d);
    return true;
}

static size_t vram_copy(const void *restrict ctx, uint16_t addr, size_t count,
                        uint8_t dest[restrict count])
{
    // addr=[$2000-$3FFF]
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    return mem_copy(ctx, addr, count, dest);
}

static bool create_mbus(struct aldo_nes001 *self)
{
    // TODO: partitions so far:
    /*
     * CPU Memory Map
     * 16-bit Address Space = 64KB
     *   $0000 - $1FFF: 2KB RAM mirrored to 8KB
     *   $2000 - $3FFF: 8 PPU registers mirrored to 8KB
     *   $4000 - $401F: APU, DMA, Joypads, unused processor test functionality
     *   $4020 - $7FFF: unmapped
     *   $8000 - $FFFF: 32KB Cart
     */
    self->extends.cpu.mbus = aldo_bus_new(ALDO_BITWIDTH_64KB, 5,
                                          ALDO_MEMBLOCK_8KB,
                                          ALDO_MEMBLOCK_16KB,
                                          ALDO_MEMBLOCK_16KB + 0x20,
                                          ALDO_MEMBLOCK_32KB);
    if (!self->extends.cpu.mbus) return false;

    auto r = aldo_bus_set(self->extends.cpu.mbus, 0, (struct aldo_busdevice){
        ram_read,
        ram_write,
        ram_copy,
        self->ram,
    });
    (void)r, assert(r);
    aldo_apu_connect(&self->apu, &self->extends.cpu);
    return true;
}

static bool create_vbus(struct aldo_nes001 *self)
{
    /*
     * PPU Memory Map
     * 14-bit Address Space = 16KB
     *   $0000 - $1FFF: 8KB CHR ROM/RAM, 2 pattern tables mapped to cartridge
     *   $2000 - $3FFF: 2KB RAM mirrored to 8KB, nametable-mirroring uses 4KB
     *                  of address space; never writes to $3F00 - $3FFF; uses
     *                  nametable vertical-mirroring by default, though the
     *                  cartridge may override this behavior.
     *   $3F00 - $3FFF: 32B Palette RAM mirrored to 256B; internal to the PPU
     *                  and thus not on the video bus, but reads do leak
     *                  through to the underlying VRAM.
     */
    self->ppu.vbus = aldo_bus_new(ALDO_BITWIDTH_16KB, 2, ALDO_MEMBLOCK_8KB);
    if (!self->ppu.vbus) return false;

    auto r = aldo_bus_set(self->ppu.vbus, ALDO_MEMBLOCK_8KB,
                          (struct aldo_busdevice){
        vram_read,
        vram_write,
        vram_copy,
        self->vram,
    });
    (void)r, assert(r);
    return true;
}

[[maybe_unused]]
static bool assert_vbus(aldo_cart* cart, bool connected)
{
    if (connected) return true;

    struct aldo_cartinfo info;
    aldo_cart_getinfo(cart, &info);
    return info.format != ALDO_CRTF_INES;
}

static void connect_cart(struct aldo_nes001 *self, aldo_cart *c)
{
    self->extends.cart = c;
    auto r = aldo_cart_mbus_connect(self->extends.cart, self->extends.cpu.mbus);
    (void)r, assert(r);
    r = aldo_cart_vbus_connect(self->extends.cart, self->ppu.vbus);
    (void)r, assert(assert_vbus(self->extends.cart, r));
    aldo_debug_sync_bus(self->extends.dbg);
}

static void disconnect_cart(struct aldo_nes001 *self)
{
    // Debugger may have been attached to a cart-less CPU bus so reset
    // debugger even if there is no existing cart.
    aldo_debug_reset(self->extends.dbg);
    if (!self->extends.cart) return;
    aldo_cart_vbus_disconnect(self->extends.cart, self->ppu.vbus);
    aldo_cart_mbus_disconnect(self->extends.cart, self->extends.cpu.mbus);
    self->extends.cart = nullptr;
}

static bool setup(struct aldo_nes001 *self)
{
    if (!create_mbus(self)) return false;
    if (!create_vbus(self)) return false;
    aldo_ppu_connect(&self->ppu, self->extends.cpu.mbus);
    return true;
}

static void teardown(struct aldo_nes001 *self)
{
    disconnect_cart(self);
    aldo_debug_cpu_disconnect(self->extends.dbg);
    aldo_bus_free(self->ppu.vbus);
    aldo_bus_free(self->extends.cpu.mbus);
}

static void set_ppu_pins(struct aldo_nes001 *self)
{
    // interrupt lines are active low
    self->ppu.signal.rst = !self->extends.probe.rst;
    // Pull PPU's CPU R/W signal back up if CPU is no longer pulling it
    // low (pulled low by PPU register writes).
    self->ppu.signal.rw |= self->extends.cpu.signal.rw;
}

static void set_screen_dot(struct aldo_nes001 *self)
{
    auto c = aldo_ppu_screendot(&self->ppu);
    if (c.dot < 0) return;

    auto screendot = (size_t)(c.dot + (c.line * Aldo_NesScreenWidth));
    assert(screendot < aldo_arrsz(self->vbufs[0]));
    self->vbufs[self->vbuf][screendot] = self->ppu.pxpl.px;
}

static void set_cpu_pins(struct aldo_nes001 *self)
{
    self->extends.cpu.signal.rdy = self->extends.probe.rdy && self->apu.signal.rdy;
    // interrupt lines are active low
    self->extends.cpu.signal.irq = !self->extends.probe.irq;
    self->extends.cpu.signal.nmi = !self->extends.probe.nmi && self->ppu.signal.intr;
    self->extends.cpu.signal.rst = !self->extends.probe.rst;
}

//
// MARK: - Snapshotting
//

static void snapshot_bus(const struct aldo_nes001 *self, struct aldo_snapshot *snp)
{
    aldo_apu_snapshot(&self->apu, snp);
    aldo_ppu_bus_snapshot(&self->ppu, snp);
}

static void snapshot_gfx(struct aldo_nes001 *self, struct aldo_snapshot *snp)
{
    aldo_ppu_vid_snapshot(&self->ppu, snp);
    if (self->extends.cart) {
        aldo_cart_snapshot(self->extends.cart, snp);
    }
}

static void snapshot_screen(struct aldo_nes001 *self, struct aldo_snapshot *snp)
{
    assert(snp->video != nullptr);

    snp->video->screen = self->vbufs[!self->vbuf];
    snp->video->newframe = true;
}

static void snapshot_video(struct aldo_nes001 *self, struct aldo_snapshot *snp, bool framedone)
{
    if (aldo_ppu_gfxsnp_dot(&self->ppu)) {
        snapshot_gfx(self, snp);
    }
    if (framedone) {
        snapshot_screen(self, snp);
    }
}

static void snapshot_core(struct aldo_nes001 *self, struct aldo_snapshot *snp)
{
    if (!snp) return;

    aldo_apu_snapshot(&self->apu, snp);
    aldo_ppu_bus_snapshot(&self->ppu, snp);
    snp->mem.ram = self->ram;
    snp->mem.vram = self->vram;
}

static void reset_snapshot(struct aldo_snapshot *snp)
{
    if (!snp) return;

    snp->video->newframe = false;
}

static void init_snapshot(struct aldo_nes001 *self, struct aldo_snapshot *snp)
{
    snapshot_core(self, snp);
    snapshot_gfx(self, snp);
    snapshot_screen(self, snp);
}

//
// MARK: - Clocking
//

// trace the just-fetched instruction
static void instruction_trace(struct aldo_nes001 *self,
                              const struct aldo_clock *clock, int adjustment)
{
    if (!self->extends.tracelog || self->extends.tracefailed
        || !self->extends.cpu.signal.sync) return;

    struct aldo_snapshot snp = {};
    aldo_cpu_snapshot(&self->extends.cpu, &snp);
    aldo_ppu_bus_snapshot(&self->ppu, &snp);
    // Trace the cycle/pixel count up to the current instruction so
    // do NOT count the just-executed instruction fetch cycle.
    self->extends.tracefailed = !aldo_trace_line(self->extends.tracelog,
                                                 adjustment,
                                                 clock->cycles, &self->extends.cpu,
                                                 &self->ppu, self->extends.dbg,
                                                 &snp);
}

static bool clock_ppu(struct aldo_nes001 *self, struct aldo_clock *clock)
{
    auto framedone = aldo_ppu_cycle(&self->ppu);
    clock->frames += (uint64_t)framedone;
    --clock->budget;
    set_ppu_pins(self);
    set_screen_dot(self);
    self->vbuf ^= framedone;
    snapshot_video(self, self->extends.snp, framedone);

    switch (self->extends.mode) {
    case ALDO_EXC_SUBCYCLE:
        aldo_console_halt(&self->extends, true);
        break;
    case ALDO_EXC_STEP:
        if (clock->rate_factor == aldo_nes_frame_factor()) {
            aldo_console_halt(&self->extends, framedone);
        }
        break;
    default:
        break;
    }

    return ++clock->subcycle >= Aldo_PpuRatio;
}

static void clock_cpu(struct aldo_nes001 *self, struct aldo_clock *clock)
{
    auto cycles = aldo_apu_cycle(&self->apu);
    set_cpu_pins(self);
    clock->subcycle = 0;
    clock->cycles += (uint64_t)cycles;
    instruction_trace(self, clock, -cycles);
}

//
// MARK: - Public Interface
//

const size_t
    Aldo_NesSize = sizeof(struct aldo_nes001),
    Aldo_NesRamSize = aldo_arrsz(((struct aldo_nes001 *)nullptr)->ram);

bool aldo_nes_init(aldo_nes *self)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_NES);

    // NES does not support Binary-Coded Decimal mode
    self->vbuf = self->extends.cpu.bcd = false;

    // uninitialized vbuffer can have out-of-range palette values
    for (size_t i = 0; i < aldo_arrsz(self->vbufs); ++i) {
        aldo_memclr(self->vbufs[i]);
    }
    return setup(self);
}

void aldo_nes_powerup(aldo_nes *self, bool zeroram)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_NES);

    if (zeroram) {
        aldo_memclr(self->ram);
        aldo_memclr(self->vram);
        aldo_ppu_zeroram(&self->ppu);
    }
    aldo_apu_powerup(&self->apu);
    aldo_ppu_powerup(&self->ppu);
}

void aldo_nes_disconnect(aldo_console *self)
{
    assert(self != nullptr);
    assert(self->type == ALDO_CONSOLE_NES);

    if (!self->cart) return;

    aldo_cart_vbus_disconnect(self->cart, ((struct aldo_nes001 *)self)->ppu.vbus);
}

void aldo_nes_free(aldo_console *self)
{
    assert(self != nullptr);
    assert(self->type == ALDO_CONSOLE_NES);

    aldo_bus_free(((struct aldo_nes001 *)self)->ppu.vbus);
}

int aldo_nes_cycle_factor()
{
    return Aldo_PpuRatio;
}

int aldo_nes_frame_factor()
{
    return Aldo_DotsPerFrame;
}

bool aldo_nes_clock(aldo_nes *self, struct aldo_clock *clock)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_NES);

    if (clock_ppu(self, clock)) {
        clock_cpu(self, clock);
        return true;
    }
    return false;
}

void aldo_nes_snapshot_init(aldo_nes *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_NES);
    assert(snp != nullptr);

    aldo_nes_snapshot_core(self, snp);
    snapshot_gfx(self, snp);
    snapshot_screen(self, snp);
}

void aldo_nes_snapshot_core(aldo_nes *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_NES);
    assert(snp != nullptr);

    aldo_apu_snapshot(&self->apu, snp);
    aldo_ppu_bus_snapshot(&self->ppu, snp);
    snp->mem.ram = self->ram;
    snp->mem.vram = self->vram;
}

void aldo_nes_snapshot_reset(aldo_nes *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_NES);
    assert(snp != nullptr);

    snp->video->newframe = false;
}

//
// MARK: - Old
//

aldo_nes *aldo_nes_new(aldo_debugger *dbg, bool bcdsupport, FILE *tracelog)
{
    assert(dbg != nullptr);

    struct aldo_nes001 *self = malloc(sizeof *self);
    if (!self) return self;

    self->extends.cart = nullptr;
    self->extends.dbg = dbg;
    self->extends.tracelog = tracelog;
    // TODO: ditch this option when aldo can emulate more than just NES
    self->extends.cpu.bcd = bcdsupport;
    self->extends.halted = self->extends.probe.rdy = true;
    self->extends.tracefailed = self->vbuf = self->extends.probe.irq = self->extends.probe.nmi =
        self->extends.probe.rst = false;
    // uninitialized vbuffer can have out-of-range palette values
    for (size_t i = 0; i < aldo_arrsz(self->vbufs); ++i) {
        aldo_memclr(self->vbufs[i]);
    }
    if (!setup(self)) {
        aldo_nes_freeold(self);
        return nullptr;
    }
    return self;
}

void aldo_nes_freeold(aldo_nes *self)
{
    assert(self != nullptr);

    teardown(self);
    free(self);
}

void aldo_nes_powerupold(aldo_nes *self, aldo_cart *c, bool zeroram)
{
    assert(self != nullptr);
    assert(self->extends.cart == nullptr);

    if (c) {
        connect_cart(self, c);
    }
    if (zeroram) {
        aldo_memclr(self->ram);
        aldo_memclr(self->vram);
        aldo_ppu_zeroram(&self->ppu);
    }
    aldo_apu_powerup(&self->apu);
    aldo_ppu_powerup(&self->ppu);
    self->extends.mode = ALDO_EXC_RUN;
}

void aldo_nes_powerdown(aldo_nes *self)
{
    assert(self != nullptr);

    aldo_nes_halt(self, true);
    disconnect_cart(self);
}

int aldo_nes_max_tcpu()
{
    return Aldo_MaxTCycle;
}

size_t aldo_nes_ram_size(aldo_nes *self)
{
    assert(self != nullptr);

    return aldo_arrsz(self->ram);
}

void aldo_nes_screen_size(int *width, int *height)
{
    assert(width != nullptr);
    assert(height != nullptr);

    *width = Aldo_NesScreenWidth;
    *height = Aldo_NesScreenHeight;
}

bool aldo_nes_bcd_support(aldo_nes *self)
{
    assert(self != nullptr);

    return self->extends.cpu.bcd;
}

bool aldo_nes_tracefailed(aldo_nes *self)
{
    assert(self != nullptr);

    return self->extends.tracefailed;
}

enum aldo_execmode aldo_nes_mode(aldo_nes *self)
{
    assert(self != nullptr);

    return self->extends.mode;
}

void aldo_nes_set_mode(aldo_nes *self, enum aldo_execmode mode)
{
    assert(self != nullptr);

    // force signed to check < 0 (underlying type may be uint)
    self->extends.mode = (int)mode < 0 ? ALDO_EXC_COUNT - 1 : mode % ALDO_EXC_COUNT;
}

bool aldo_nes_halted(aldo_nes *self)
{
    assert(self != nullptr);

    return self->extends.halted;
}

void aldo_nes_halt(aldo_nes *self, bool halt)
{
    assert(self != nullptr);

    self->extends.halted = halt;
}

bool aldo_nes_probe(aldo_nes *self, enum aldo_interrupt signal)
{
    assert(self != nullptr);

    switch (signal) {
    case ALDO_INT_IRQ:
        return self->extends.probe.irq;
    case ALDO_INT_NMI:
        return self->extends.probe.nmi;
    case ALDO_INT_RDY:
        return self->extends.probe.rdy;
    case ALDO_INT_RST:
        return self->extends.probe.rst;
    default:
        assert(((void)"INVALID NES PROBE", false));
        return false;
    }
}

void aldo_nes_set_probe(aldo_nes *self, enum aldo_interrupt signal, bool active)
{
    assert(self != nullptr);

    switch (signal) {
    case ALDO_INT_IRQ:
        self->extends.probe.irq = active;
        break;
    case ALDO_INT_NMI:
        self->extends.probe.nmi = active;
        break;
    case ALDO_INT_RDY:
        self->extends.probe.rdy = active;
        break;
    case ALDO_INT_RST:
        self->extends.probe.rst = active;
        break;
    default:
        assert(((void)"INVALID NES PROBE", false));
        break;
    }
}

void aldo_nes_clockold(aldo_nes *self, struct aldo_clock *clock)
{
    assert(self != nullptr);
    assert(clock != nullptr);

    if (aldo_nes_halted(self)) return;

    reset_snapshot(self->extends.snp);
    while (clock->budget > 0 && !aldo_nes_halted(self)) {
        if (!clock_ppu(self, clock)) continue;
        clock_cpu(self, clock);
        if (aldo_debug_break(self->extends.dbg, clock)) {
            aldo_nes_halt(self, true);
        }
    }
    snapshot_core(self, self->extends.snp);
}

int aldo_nes_cycle_factorold()
{
    return Aldo_PpuRatio;
}

int aldo_nes_frame_factorold()
{
    return Aldo_DotsPerFrame;
}

void aldo_nes_set_snapshot(aldo_nes *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);

    self->extends.snp = snp;
    init_snapshot(self, snp);
}

void aldo_nes_dumpram(aldo_nes *self, FILE *fs[static 3], bool errs[static 3])
{
    assert(self != nullptr);
    assert(fs != nullptr);
    assert(errs != nullptr);

    FILE *f;
    if ((f = fs[0])) {
        errs[0] = !aldo_memdump(self->ram, f);
    }
    if ((f = fs[1])) {
        errs[1] = !aldo_memdump(self->vram, f);
    }
    if ((f = fs[2])) {
        errs[2] = !aldo_ppu_dumpram(&self->ppu, f);
    }
}
