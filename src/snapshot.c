//
//  snapshot.c
//  Aldo
//
//  Created by Brandon Stansbury on 3/24/22.
//

#include "snapshot.h"

#include <assert.h>

void snapshot_clear(struct snapshot *snp)
{
    assert(snp != NULL);

    snp->mem.prglength = 0;
    snp->mem.ram = NULL;
}
