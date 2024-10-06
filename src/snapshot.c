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

static_assert(ALDO_PT_TILE_COUNT * ALDO_CHR_TILE_STRIDE == ALDO_MEMBLOCK_4KB,
              "Pattern table size mismatch");

void aldo_snapshot_extend(struct aldo_snapshot *snp)
{
    assert(snp != NULL);

    snp->prg.curr = malloc(sizeof *snp->prg.curr);
    snp->video = malloc(sizeof *snp->video);
}

void aldo_snapshot_cleanup(struct aldo_snapshot *snp)
{
    assert(snp != NULL);

    free(snp->video);
    free(snp->prg.curr);
}
