//
//  proc.c
//  Aldo
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "proc.h"

#include <assert.h>

//
// MARK: - Public Interface
//

void aldo_proc_powerup(struct aldo_rp2a03 *self)
{
    assert(self != nullptr);

    aldo_cpu_powerup(&self->cpu);

    // start on a get cycle (in real hardware, startup state is random)
    self->put = false;
}

int aldo_proc_cycle(struct aldo_rp2a03 *self)
{
    assert(self != nullptr);

    // TODO: run apu/dma/etc
    return aldo_cpu_cycle(&self->cpu);
}

void aldo_proc_snapshot(const struct aldo_rp2a03 *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);
    assert(snp != nullptr);

    // TODO: snapshot apu/dma/etc
    aldo_cpu_snapshot(&self->cpu, snp);
}
