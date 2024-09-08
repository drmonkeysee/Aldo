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

void snapshot_extend(struct snapshot *snp)
{
    assert(snp != NULL);

    snp->prg.curr = malloc(sizeof *snp->prg.curr);
    snp->video = malloc(sizeof *snp->video);
}

void snapshot_cleanup(struct snapshot *snp)
{
    assert(snp != NULL);

    free(snp->video);
    free(snp->prg.curr);
}
