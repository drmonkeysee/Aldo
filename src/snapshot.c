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

void snapshot_extsetup(struct snapshot *snp)
{
    assert(snp != NULL);

    snp->prg = malloc(sizeof *snp->prg);
    snp->video = malloc(sizeof *snp->video);
}

void snapshot_extcleanup(struct snapshot *snp)
{
    assert(snp != NULL);

    free(snp->video);
    free(snp->prg);
}
