//
//  snapshot.c
//  Aldo
//
//  Created by Brandon Stansbury on 8/29/24.
//

#include "snapshot.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

void snapshot_setup(struct snapshot *snp)
{
    assert(snp != NULL);

    snp->video = malloc(sizeof *snp->video);
}

void snapshot_teardown(struct snapshot *snp)
{
    assert(snp != NULL);

    free(snp->video);
}
