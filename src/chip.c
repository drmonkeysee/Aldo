//
//  chip.c
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "chip.h"

#include "snapshot.h"

#include <assert.h>

static int cycle_chip(struct aldo_rp2a03 *self)
{
    if (aldo_cpu_reset_pending(&self->cpu)) return 0;

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

    // start on a get cycle (in real hardware, startup state is random)
    self->put = false;
    // TODO: figure out what resets on RST vs powerup
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
    apu->put = self->put;

    aldo_cpu_snapshot(&self->cpu, snp);
}
