//
//  chip.c
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "chip.h"

#include "ctrlsignal.h"
#include "snapshot.h"

#include <assert.h>

static void reset(struct aldo_rp2a03 *self)
{
    self->oam.low = 0x0;
    self->oam.active = false;
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

void aldo_chip_powerup(struct aldo_rp2a03 *self)
{
    assert(self != nullptr);

    aldo_cpu_powerup(&self->cpu);

    // powerup on a get cycle (in real hardware, startup state is random)
    self->put = false;
    self->oam.dma = 0x0;
    reset(self);
}

int aldo_chip_cycle(struct aldo_rp2a03 *self)
{
    assert(self != nullptr);

    // cycle will return non-zero if DMA is running, which suspends the cpu
    return cycle_chip(self) || aldo_cpu_cycle(&self->cpu);
}

void aldo_chip_snapshot(const struct aldo_rp2a03 *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);
    assert(snp != nullptr);

    auto apu = &snp->apu;

    apu->lines.ready = self->signal.rdy;

    apu->oam.active = self->oam.active;
    apu->oam.dmahigh = self->oam.dma;
    apu->oam.dmalow = self->oam.low;

    apu->put = self->put;

    aldo_cpu_snapshot(&self->cpu, snp);
}
