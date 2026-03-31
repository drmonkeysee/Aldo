//
//  apu.c
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "apu.h"

#include "bus.h"
#include "bytes.h"
#include "snapshot.h"

#include <assert.h>

constexpr uint16_t OamData = 0x2004;

//
// MARK: - Main Bus Device (APU/DMA/Joypad registers)
//

static bool mbus_read(struct aldo_rp2a03 *self)
{
    return aldo_bus_read(self->cpu.mbus, self->addrbus, &self->databus);
}

static bool mbus_write(struct aldo_rp2a03 *self)
{
    return aldo_bus_write(self->cpu.mbus, self->addrbus, self->databus);
}

static bool read([[maybe_unused]] void *restrict ctx, uint16_t addr, [[maybe_unused]] uint8_t *restrict d)
{
    // addr=[$4000-$401F]
    assert(ALDO_MEMBLOCK_16KB <= addr && addr < ALDO_MEMBLOCK_16KB + 0x20);

    return false;
}

static bool write(void *ctx, uint16_t addr, uint8_t d)
{
    // addr=[$4000-$401F]
    assert(ALDO_MEMBLOCK_16KB <= addr && addr < ALDO_MEMBLOCK_16KB + 0x20);

    struct aldo_rp2a03 *apu = ctx;
    switch (addr & 0x1f) {
    case 0x14:  // OAMDMA
        apu->oam = (typeof(apu->oam)){.s = ALDO_SIG_PENDING, .hi = d};
        break;
    default:    // Unimplemented test functionality
        return false;
    }

    return true;
}

//
// MARK: - DMA
//

static int oam_dma(struct aldo_rp2a03 *self)
{
    if (!aldo_cpu_suspended(&self->cpu)) return 0;

    switch (self->oam.s) {
    case ALDO_SIG_PENDING:
        if (!self->put) {
            self->addrbus = aldo_bytowr(self->oam.lo, self->oam.hi);
            mbus_read(self);
            self->oam.s = ALDO_SIG_COMMITTED;
        }
        return 1;
    case ALDO_SIG_COMMITTED:
        if (self->put) {
            self->addrbus = OamData;
            mbus_write(self);
            self->oam.s = ++self->oam.lo
                            ? ALDO_SIG_PENDING
                            : ALDO_SIG_SERVICED;
        }
        return 1;
    case ALDO_SIG_SERVICED:
        // Extra state in order to set RDY *after* the last DMA write, so CPU
        // resumption repeats its last read on this cycle.
        self->oam.s = ALDO_SIG_CLEAR;
        break;
    default:
        break;
    }

    return 0;
}

//
// MARK: - Internal Operations
//

static void reset(struct aldo_rp2a03 *self)
{
    self->oam.lo = 0x0;
    self->oam.s = ALDO_SIG_CLEAR;
    self->signal.rdy = true;
}

static bool reset_held(struct aldo_rp2a03 *self)
{
    if (aldo_cpu_reset_pending(&self->cpu)) return true;

    // CPU is actively resetting
    if (self->cpu.rst == ALDO_SIG_COMMITTED) {
        reset(self);
    }
    return false;
}

static int cycle_chip(struct aldo_rp2a03 *self)
{
    if (reset_held(self)) return 0;

    auto cycle = oam_dma(self);

    self->signal.rdy = self->oam.s == ALDO_SIG_CLEAR;
    self->put = !self->put;
    return cycle;
}

//
// MARK: - Public Interface
//

void aldo_apu_connect(struct aldo_rp2a03 *self)
{
    assert(self != nullptr);
    assert(self->cpu.mbus != nullptr);

    auto r = aldo_bus_set(self->cpu.mbus, ALDO_MEMBLOCK_16KB, (struct aldo_busdevice){
        .read = read,
        .write = write,
        .ctx = self,
    });
    (void)r, assert(r);
}

void aldo_apu_powerup(struct aldo_rp2a03 *self)
{
    assert(self != nullptr);

    aldo_cpu_powerup(&self->cpu);

    // powerup on a get cycle (in real hardware, put/get cycle is random)
    self->put = false;
    self->oam.hi = 0x0;
    reset(self);
}

int aldo_apu_cycle(struct aldo_rp2a03 *self)
{
    assert(self != nullptr);

    // cycle will return non-zero if DMA is running, which suspends the cpu
    return cycle_chip(self) || aldo_cpu_cycle(&self->cpu);
}

void aldo_apu_snapshot(const struct aldo_rp2a03 *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);
    assert(snp != nullptr);

    auto apu = &snp->apu;

    apu->oam.state = self->oam.s;
    apu->oam.dmahigh = self->oam.hi;
    apu->oam.dmalow = self->oam.lo;

    apu->addressbus = self->addrbus;
    apu->databus = self->databus;

    apu->lines.ready = self->signal.rdy;

    apu->put = self->put;

    aldo_cpu_snapshot(&self->cpu, snp);
}
