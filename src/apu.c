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

//
// MARK: - Main Bus Device (APU/DMA/Joypad registers)
//

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
        apu->oam = (typeof(apu->oam)){.s = ALDO_SIG_DETECTED, .dma = d};
        break;
    default:    // Unimplemented test functionality
        return false;
    }

    return true;
}

//
// MARK: - Internal Operations
//

static void reset(struct aldo_rp2a03 *self)
{
    self->oam.low = 0x0;
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

    self->put = !self->put;
    return 0;
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

    // powerup on a get cycle (in real hardware, startup state is random)
    self->put = false;
    self->oam.dma = 0x0;
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

    apu->lines.ready = self->signal.rdy;

    apu->oam.state = self->oam.s;
    apu->oam.dmahigh = self->oam.dma;
    apu->oam.dmalow = self->oam.low;

    apu->put = self->put;

    aldo_cpu_snapshot(&self->cpu, snp);
}
