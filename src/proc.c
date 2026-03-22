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
