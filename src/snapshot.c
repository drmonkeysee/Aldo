//
//  snapshot.c
//  Aldo
//
//  Created by Brandon Stansbury on 8/29/24.
//

#include "snapshot.h"

#include "bytes.h"

#include <assert.h>

static_assert(AldoPtTileCount * AldoChrTileStride == ALDO_MEMBLOCK_4KB,
              "Pattern table size mismatch");

void aldo_snapshot_clear(struct aldo_snapshot *snp)
{
    assert(snp != nullptr);

    snp->mem = (typeof(snp->mem)){};
    snp->video.screen = nullptr;
}
