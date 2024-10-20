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

bool aldo_snapshot_extend(struct aldo_snapshot *snp)
{
    assert(snp != NULL);

    snp->prg.curr = malloc(sizeof *snp->prg.curr);
    if (!snp->prg.curr) return false;

    snp->video = malloc(sizeof *snp->video);
    if (!snp->video) {
        aldo_snapshot_cleanup(snp);
        return false;
    }
    return true;
}

void aldo_snapshot_cleanup(struct aldo_snapshot *snp)
{
    assert(snp != NULL);

    free(snp->video);
    free(snp->prg.curr);
}
