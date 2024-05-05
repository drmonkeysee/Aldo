//
//  ppu.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ppu.h"

#include <assert.h>
#include <stddef.h>

//
// Public Interface
//

void ppu_powerup(struct rp2c02 *self)
{
    assert(self != NULL);

    self->odd = self->w = false;
}
