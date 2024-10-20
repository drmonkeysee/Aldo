//
//  mappers.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#include "mappers.h"

#include "bytes.h"
#include "cart.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

struct raw_mapper {
    struct aldo_mapper vtable;
    uint8_t *rom;
};

struct ines_mapper {
    struct aldo_nesmapper vtable;
    uint8_t *prg, *chr, *wram, id;
    bool chrram, ptstale;
};

struct ines_000_mapper {
    struct ines_mapper super;
    size_t blockcount;
};

static int load_blocks(uint8_t *restrict *mem, size_t size, FILE *f)
{
    if (!(*mem = calloc(size, sizeof **mem))) return ALDO_CART_ERR_ERNO;
    fread(*mem, sizeof **mem, size, f);
    if (feof(f)) return ALDO_CART_ERR_EOF;
    if (ferror(f)) return ALDO_CART_ERR_IO;
    return 0;
}

//
// MARK: - Common Implementation
//

static void mem_load(uint8_t *restrict d, const uint8_t *restrict mem,
                     uint16_t addr, uint16_t mask)
{
    *d = mem[addr & mask];
}

// TODO: once we introduce wram or additional mappers this implementation won't
// be so common anymore.
static void clear_prg_device(aldo_bus *b)
{
    aldo_bus_clear(b, ALDO_MEMBLOCK_32KB);
}

static void clear_chr_device(aldo_bus *b)
{
    aldo_bus_clear(b, 0);
}

static void fill_pattern_table(size_t tile_count,
                               uint16_t table[tile_count][ALDO_CHR_TILE_DIM],
                               const struct aldo_blockview *bv)
{
    assert(tile_count <= ALDO_PT_TILE_COUNT);
    assert(bv->size >= tile_count * ALDO_CHR_TILE_STRIDE);

    for (size_t tile = 0; tile < tile_count; ++tile) {
        for (size_t row = 0; row < ALDO_CHR_TILE_DIM; ++row) {
            size_t idx = row + (tile * ALDO_CHR_TILE_STRIDE);
            assert(idx < bv->size - ALDO_CHR_TILE_DIM);
            uint8_t
                plane0 = bv->mem[idx],
                plane1 = bv->mem[idx + ALDO_CHR_TILE_DIM];
            table[tile][row] = aldo_byteshuffle(plane0, plane1);
        }
    }
}

//
// MARK: - Raw ROM Image Implementation
//

static bool raw_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    mem_load(d, ctx, addr, ALDO_ADDRMASK_32KB);
    return true;
}

static size_t raw_copy(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    return aldo_bytecopy_bank(ctx, ALDO_BITWIDTH_32KB, addr, count, dest);
}

static void raw_dtor(struct aldo_mapper *self)
{
    assert(self != NULL);

    struct raw_mapper *m = (struct raw_mapper *)self;
    free(m->rom);
    free(m);
}

static const uint8_t *raw_prgrom(const struct aldo_mapper *self)
{
    assert(self != NULL);

    return ((const struct raw_mapper *)self)->rom;
}

static bool raw_mbus_connect(struct aldo_mapper *self, aldo_bus *b)
{
    assert(self != NULL);

    return aldo_bus_set(b, ALDO_MEMBLOCK_32KB, (struct aldo_busdevice){
        .read = raw_read,
        .copy = raw_copy,
        .ctx = ((struct raw_mapper *)self)->rom,
    });
}

//
// MARK: - iNES Implementation
//

static void ines_dtor(struct aldo_mapper *self)
{
    assert(self != NULL);

    struct ines_mapper *m = (struct ines_mapper *)self;
    free(m->chr);
    free(m->prg);
    free(m->wram);
    free(m);
}

static const uint8_t *ines_prgrom(const struct aldo_mapper *self)
{
    assert(self != NULL);

    return ((const struct ines_mapper *)self)->prg;
}

static const uint8_t *ines_chrrom(const struct aldo_mapper *self)
{
    assert(self != NULL);

    return ((const struct ines_mapper *)self)->chr;
}

static bool ines_unimplemented_mbus_connect(struct aldo_mapper *self,
                                            aldo_bus *b)
{
    (void)self;
    clear_prg_device(b);
    return true;
}

static bool ines_unimplemented_vbus_connect(struct aldo_mapper *self,
                                            aldo_bus *b)
{
    (void)self;
    clear_chr_device(b);
    return true;
}

//
// MARK: - iNES 000
//

static bool ines_000_read(void *restrict ctx, uint16_t addr,
                          uint8_t *restrict d)
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    const struct ines_000_mapper *m = ctx;
    uint16_t mask = m->blockcount == 2
                    ? ALDO_ADDRMASK_32KB
                    : ALDO_ADDRMASK_16KB;
    *d = m->super.prg[addr & mask];
    return true;
}

static size_t ines_000_copy(const void *restrict ctx, uint16_t addr,
                            size_t count, uint8_t dest[restrict count])
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    const struct ines_000_mapper *m = ctx;
    uint16_t width = m->blockcount == 2
                        ? ALDO_BITWIDTH_32KB
                        : ALDO_BITWIDTH_16KB;
    return aldo_bytecopy_bank(m->super.prg, width, addr, count, dest);
}

static bool ines_000_vread(void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    mem_load(d, ((const struct ines_mapper *)ctx)->chr, addr,
             ALDO_ADDRMASK_8KB);
    return true;
}

static bool ines_000_vwrite(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    struct ines_mapper *m = ctx;
    m->chr[addr & ALDO_ADDRMASK_8KB] = d;
    m->ptstale = true;
    return true;
}

// TODO: binding to $8000 is too simple; WRAM needs at least $6000, and the
// CPU memory map defines start of cart mapping at $4020; the most complex
// mappers need access to entire 64KB address space in order to snoop on
// all CPU activity. Similar rules hold for PPU.
static bool ines_000_mbus_connect(struct aldo_mapper *self, aldo_bus *b)
{
    assert(self != NULL);

    return aldo_bus_set(b, ALDO_MEMBLOCK_32KB, (struct aldo_busdevice){
        .read = ines_000_read,
        .copy = ines_000_copy,
        .ctx = self,
    });
}

static bool ines_000_vbus_connect(struct aldo_mapper *self, aldo_bus *b)
{
    assert(self != NULL);

    struct ines_mapper *m = (struct ines_mapper *)self;
    return aldo_bus_set(b, 0, (struct aldo_busdevice){
        .read = ines_000_vread,
        .write = m->chrram ? ines_000_vwrite : NULL,
        .ctx = m,
    });
}

static void ines_000_snapshot(struct aldo_mapper *self,
                              struct aldo_snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);
    assert(snp->video != NULL);

    struct ines_mapper *m = (struct ines_mapper *)self;
    if (!m->ptstale) return;

    struct aldo_blockview bv = {
        .mem = m->chr,
        .size = ALDO_MEMBLOCK_4KB,
    };
    fill_pattern_table(ALDO_PT_TILE_COUNT, snp->video->pattern_tables.left,
                       &bv);
    bv.mem += bv.size;
    ++bv.ord;
    fill_pattern_table(ALDO_PT_TILE_COUNT, snp->video->pattern_tables.right,
                       &bv);
    m->ptstale = false;
}

//
// MARK: - Public Interface
//

int aldo_mapper_raw_create(struct aldo_mapper **m, FILE *f)
{
    assert(m != NULL);
    assert(f != NULL);

    struct raw_mapper *self = malloc(sizeof *self);
    if (!self) return ALDO_CART_ERR_ERNO;

    *self = (struct raw_mapper){
        .vtable = {
            .dtor = raw_dtor,
            .prgrom = raw_prgrom,
            .mbus_connect = raw_mbus_connect,
            .mbus_disconnect = clear_prg_device,
        },
    };

    // TODO: assume a 32KB ROM file (can i do mirroring later?)
    int err = load_blocks(&self->rom, ALDO_MEMBLOCK_32KB, f);
    if (err == 0) {
        *m = (struct aldo_mapper *)self;
    } else {
        self->vtable.dtor((struct aldo_mapper *)self);
    }
    return err;
}

int aldo_mapper_ines_create(struct aldo_mapper **m,
                            struct aldo_ines_header *header, FILE *f)
{
    assert(m != NULL);
    assert(header != NULL);
    assert(f != NULL);

    struct ines_mapper *self;
    if (header->mapper_id == 0) {
        if (!(self = malloc(sizeof(struct ines_000_mapper))))
            return ALDO_CART_ERR_ERNO;

        *self = (struct ines_mapper){
            .vtable = {
                .extends = {.mbus_connect = ines_000_mbus_connect},
                .vbus_connect = ines_000_vbus_connect,
                .snapshot = ines_000_snapshot,
            },
        };
        assert(header->prg_blocks <= 2);
        ((struct ines_000_mapper *)self)->blockcount = header->prg_blocks;
        header->mapper_implemented = true;
    } else {
        if (!(self = malloc(sizeof *self))) return ALDO_CART_ERR_ERNO;

        *self = (struct ines_mapper){
            .vtable = {
                .extends = {.mbus_connect = ines_unimplemented_mbus_connect},
                .vbus_connect = ines_unimplemented_vbus_connect,
            },
        };
        header->mapper_implemented = false;
    }
    struct aldo_mapper *base = &self->vtable.extends;
    base->dtor = ines_dtor;
    base->prgrom = ines_prgrom;
    base->mbus_disconnect = clear_prg_device;
    self->vtable.vbus_disconnect = clear_chr_device;
    if (header->chr_blocks > 0) {
        self->vtable.chrrom = ines_chrrom;
    }
    self->ptstale = true;
    self->id = header->mapper_id;

    int err;
    if (header->trainer) {
        // NOTE: skip 512 bytes of trainer data
        if (fseek(f, 512, SEEK_CUR) != 0) {
            err = ALDO_CART_ERR_ERNO;
            goto cleanup;
        }
    }

    if (header->wram) {
        size_t sz = (header->wram_blocks == 0
                     ? 1
                     : header->wram_blocks) * ALDO_MEMBLOCK_8KB;
        if (!(self->wram = calloc(sz, sizeof *self->wram))) {
            err = ALDO_CART_ERR_ERNO;
            goto cleanup;
        }
    }

    err = load_blocks(&self->prg, header->prg_blocks * ALDO_MEMBLOCK_16KB, f);
    if (err != 0) goto cleanup;

    if (header->chr_blocks == 0) {
        // TODO: this size is controlled by the mapper in many cases
        if (!(self->chr = calloc(ALDO_MEMBLOCK_8KB, sizeof *self->chr))) {
            err = ALDO_CART_ERR_ERNO;
            goto cleanup;
        }
        self->chrram = true;
    } else {
        err = load_blocks(&self->chr, header->chr_blocks * ALDO_MEMBLOCK_8KB,
                          f);
    }

cleanup:
    if (err == 0) {
        *m = (struct aldo_mapper *)self;
    } else {
        base->dtor((struct aldo_mapper *)self);
    }
    return err;
}
