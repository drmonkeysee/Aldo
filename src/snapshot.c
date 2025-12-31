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

static_assert(AldoPtTileCount * AldoChrTileStride == ALDO_MEMBLOCK_4KB,
              "Pattern table size mismatch");

bool aldo_snapshot_extend(struct aldo_snapshot *snp)
{
    assert(snp != nullptr);

    if (!(snp->prg.curr = malloc(sizeof *snp->prg.curr))) return false;
    if (!(snp->video = malloc(sizeof *snp->video))) {
        aldo_snapshot_cleanup(snp);
        return false;
    }
    return true;
}

void aldo_snapshot_cleanup(struct aldo_snapshot *snp)
{
    assert(snp != nullptr);

    free(snp->video);
    free(snp->prg.curr);
}
