//
//  snapshot.c
//  Aldo
//
//  Created by Brandon Stansbury on 8/29/24.
//

#include "snapshot.h"

#include "bytes.h"

#include <assert.h>
#include <stdlib.h>

static_assert(CHR_PAT_TILES * CHR_TILE_STRIDE == MEMBLOCK_4KB,
              "Pattern table size mismatch");

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
