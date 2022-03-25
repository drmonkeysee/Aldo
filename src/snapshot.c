//
//  snapshot.c
//  Aldo
//
//  Created by Brandon Stansbury on 3/24/22.
//

#include "snapshot.h"

#include <assert.h>

void snapshot_clear(struct console_state *snapshot)
{
    assert(snapshot != NULL);

    snapshot->mem.prglength = 0;
    snapshot->mem.ram = NULL;
    snapshot->cart.info = NULL;
}
