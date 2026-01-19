//
//  mappers.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#include "mappers.h"

#include "bus.h"
#include "bytes.h"
#include "cart.h"
#include "ppu.h"
#include "snapshot.h"

#include <assert.h>
#include <stddef.h>

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
    struct aldo_busdevice vrbd;
    size_t blockcount;
    bool hmirroring;
};

static int load_blocks(uint8_t *restrict *mem, size_t size, FILE *f)
{
    if (!(*mem = calloc(size, sizeof **mem))) return ALDO_CART_ERR_ERNO;
    if (fread(*mem, sizeof **mem, size, f) < size) {
        if (feof(f)) return ALDO_CART_ERR_EOF;
        if (ferror(f)) return ALDO_CART_ERR_IO;
        return ALDO_CART_ERR_UNKNOWN;
    }
    return 0;
}

//
// MARK: - Common Implementation
//

// Shifting the high-nametable bit (11) right 1 position (10) effectively
// divides the nametable selection in half, folding the two lower nametables
// into [0x000-0x3FF] and the two upper nametables into [0x400-0x7FF], which is
// the definition of horizontal mirroring once the 14-bit VRAM address is
// masked down to the 2KB (0x800) of addressable VRAM.
static uint16_t hmirror_addr(uint16_t addr)
{
    return (uint16_t)((addr & ~0xc00) | ((addr & ALDO_MEMBLOCK_2KB) >> 1));
}

static void mem_load(uint8_t *restrict d, const uint8_t *restrict mem,
                     uint16_t addr, uint16_t mask)
{
    *d = mem[addr & mask];
}

// TODO: once we introduce wram or additional mappers this implementation won't
// be so common anymore.
static void clear_prg_device(aldo_bus *b)
{
    auto r = aldo_bus_clear(b, ALDO_MEMBLOCK_32KB);
    (void)r, assert(r);
}

static void clear_chr_device(aldo_bus *b)
{
    auto r = aldo_bus_clear(b, 0);
    (void)r, assert(r);
}

static void fill_pattern_table(size_t tile_count,
                               uint16_t table[tile_count][AldoChrTileDim],
                               const struct aldo_blockview *bv)
{
    assert(tile_count <= AldoPtTileCount);
    assert(bv->size >= tile_count * AldoChrTileStride);

    for (size_t tile = 0; tile < tile_count; ++tile) {
        for (size_t row = 0; row < AldoChrTileDim; ++row) {
            size_t idx = row + (tile * AldoChrTileStride);
            assert(idx < bv->size - AldoChrTileDim);
            uint8_t
                plane0 = bv->mem[idx],
                plane1 = bv->mem[idx + AldoChrTileDim];
            table[tile][row] = aldo_byteshuffle(plane0, plane1);
        }
    }
}

//
// MARK: - Raw ROM Image Implementation
//

static bool raw_prgr(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    mem_load(d, ctx, addr, ALDO_ADDRMASK_32KB);
    return true;
}

static size_t raw_prgc(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    return aldo_bytecopy_bank(ctx, ALDO_BITWIDTH_32KB, addr, count, dest);
}

static void raw_dtor(struct aldo_mapper *self)
{
    assert(self != nullptr);

    auto m = (struct raw_mapper *)self;
    free(m->rom);
    free(m);
}

static const uint8_t *raw_prgrom(const struct aldo_mapper *self)
{
    assert(self != nullptr);

    return ((const struct raw_mapper *)self)->rom;
}

static bool raw_mbus_connect(struct aldo_mapper *self, aldo_bus *b)
{
    assert(self != nullptr);

    return aldo_bus_set(b, ALDO_MEMBLOCK_32KB, (struct aldo_busdevice){
        .read = raw_prgr,
        .copy = raw_prgc,
        .ctx = ((struct raw_mapper *)self)->rom,
    });
}

//
// MARK: - iNES Implementation
//

static void ines_dtor(struct aldo_mapper *self)
{
    assert(self != nullptr);

    auto m = (struct ines_mapper *)self;
    free(m->chr);
    free(m->prg);
    free(m->wram);
    free(m);
}

static const uint8_t *ines_prgrom(const struct aldo_mapper *self)
{
    assert(self != nullptr);

    return ((const struct ines_mapper *)self)->prg;
}

static const uint8_t *ines_chrrom(const struct aldo_mapper *self)
{
    assert(self != nullptr);

    return ((const struct ines_mapper *)self)->chr;
}

static bool ines_unimplemented_mbus_connect(struct aldo_mapper *, aldo_bus *b)
{
    clear_prg_device(b);
    return true;
}

static bool ines_unimplemented_vbus_connect(struct aldo_mapper *, aldo_bus *b)
{
    clear_chr_device(b);
    return true;
}

//
// MARK: - iNES 000
//

static bool ines_000_prgr(void *restrict ctx, uint16_t addr,
                          uint8_t *restrict d)
{
    // addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    const struct ines_000_mapper *m = ctx;
    uint16_t mask = m->blockcount == 2
                    ? ALDO_ADDRMASK_32KB
                    : ALDO_ADDRMASK_16KB;
    mem_load(d, m->super.prg, addr, mask);
    return true;
}

static size_t ines_000_prgc(const void *restrict ctx, uint16_t addr,
                            size_t count, uint8_t dest[restrict count])
{
    // addr=[$8000-$FFFF]
    assert(addr > ALDO_ADDRMASK_32KB);

    const struct ines_000_mapper *m = ctx;
    uint16_t width = m->blockcount == 2
                        ? ALDO_BITWIDTH_32KB
                        : ALDO_BITWIDTH_16KB;
    return aldo_bytecopy_bank(m->super.prg, width, addr, count, dest);
}

static bool ines_000_chrr(void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    // addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    mem_load(d, ((const struct ines_mapper *)ctx)->chr, addr,
             ALDO_ADDRMASK_8KB);
    return true;
}

static bool ines_000_chrw(void *ctx, uint16_t addr, uint8_t d)
{
    // addr=[$0000-$1FFF]
    assert(addr < ALDO_MEMBLOCK_8KB);

    struct ines_mapper *m = ctx;
    m->chr[addr & ALDO_ADDRMASK_8KB] = d;
    m->ptstale = true;
    return true;
}

static bool ines_000_vrmr(void *restrict ctx, uint16_t addr,
                          uint8_t *restrict d)
{
    // addr=[$2000-$3FFF]
    // Palette reads still hit the VRAM bus and affect internal PPU
    // buffers, so the full 8KB range is valid input.
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    const struct ines_000_mapper *m = ctx;
    return m->vrbd.read(m->vrbd.ctx, m->hmirroring ? hmirror_addr(addr) : addr, d);
}

static bool ines_000_vrmw(void *ctx, uint16_t addr, uint8_t d)
{
    // addr=[$2000-$3EFF]
    // writes to palette RAM should never hit the video bus
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < Aldo_PaletteStartAddr);

    const struct ines_000_mapper *m = ctx;
    return m->vrbd.write(m->vrbd.ctx,
                         m->hmirroring ? hmirror_addr(addr) : addr, d);
}

static size_t ines_000_vrmc(const void *restrict ctx, uint16_t addr,
                            size_t count, uint8_t dest[restrict count])
{
    // addr=[$2000-$3FFF]
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    const struct ines_000_mapper *m = ctx;
    return m->vrbd.copy(m->vrbd.ctx, addr, count, dest);
}

// TODO: binding to $8000 is too simple; WRAM needs at least $6000, and the
// CPU memory map defines start of cart mapping at $4020; the most complex
// mappers need access to entire 64KB address space in order to snoop on
// all CPU activity. Similar rules hold for PPU.
static bool ines_000_mbus_connect(struct aldo_mapper *self, aldo_bus *b)
{
    assert(self != nullptr);

    return aldo_bus_set(b, ALDO_MEMBLOCK_32KB, (struct aldo_busdevice){
        .read = ines_000_prgr,
        .copy = ines_000_prgc,
        .ctx = self,
    });
}

static bool ines_000_vbus_connect(struct aldo_mapper *self, aldo_bus *b)
{
    assert(self != nullptr);

    auto m = (struct ines_000_mapper *)self;
    return aldo_bus_set(b, 0, (struct aldo_busdevice){
        .read = ines_000_chrr,
        .write = m->super.chrram ? ines_000_chrw : nullptr,
        .ctx = m,
    })
    // TODO: if this fails in release mode, assert won't stop it and disconnect
    // will treat the base VRAM bus device as a mapper device and
    // (probably... hopefully) crash.
    && aldo_bus_swap(b, ALDO_MEMBLOCK_8KB, (struct aldo_busdevice){
        ines_000_vrmr,
        ines_000_vrmw,
        ines_000_vrmc,
        m,
    }, &m->vrbd);
}

static void ines_000_vbus_disconnect(aldo_bus *b)
{
    struct aldo_busdevice bd;
    auto r = aldo_bus_swap(b, ALDO_MEMBLOCK_8KB, (struct aldo_busdevice){}, &bd);
    (void)r, assert(r);
    r = aldo_bus_set(b, ALDO_MEMBLOCK_8KB,
                     ((struct ines_000_mapper *)bd.ctx)->vrbd);
    (void)r, assert(r);
    clear_chr_device(b);
}

static void ines_000_snapshot(struct aldo_mapper *self,
                              struct aldo_snapshot *snp)
{
    assert(self != nullptr);
    assert(snp != nullptr);

    auto video = snp->video;
    assert(video != nullptr);

    auto m = (struct ines_000_mapper *)self;
    video->nt.mirror = m->hmirroring ? ALDO_NTM_HORIZONTAL : ALDO_NTM_VERTICAL;

    auto b = &m->super;
    if (!b->ptstale) return;

    struct aldo_blockview bv = {
        .mem = b->chr,
        .size = ALDO_MEMBLOCK_4KB,
    };
    fill_pattern_table(aldo_arrsz(video->pattern_tables.left),
                       video->pattern_tables.left, &bv);
    bv.mem += bv.size;
    ++bv.ord;
    fill_pattern_table(aldo_arrsz(video->pattern_tables.right),
                       video->pattern_tables.right, &bv);
    b->ptstale = false;
}

//
// MARK: - Public Interface
//

int aldo_mapper_raw_create(struct aldo_mapper **m, FILE *f)
{
    assert(m != nullptr);
    assert(f != nullptr);

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
    auto err = load_blocks(&self->rom, ALDO_MEMBLOCK_32KB, f);
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
    assert(m != nullptr);
    assert(header != nullptr);
    assert(f != nullptr);

    struct ines_mapper *self;
    if (header->mapper_id == 0) {
        if (!(self = malloc(sizeof(struct ines_000_mapper))))
            return ALDO_CART_ERR_ERNO;

        *self = (struct ines_mapper){
            .vtable = {
                .extends = {.mbus_connect = ines_000_mbus_connect},
                .vbus_connect = ines_000_vbus_connect,
                .vbus_disconnect = ines_000_vbus_disconnect,
                .snapshot = ines_000_snapshot,
            },
        };
        assert(header->prg_blocks <= 2);
        auto m = (struct ines_000_mapper *)self;
        m->blockcount = header->prg_blocks;
        m->hmirroring = header->mirror == ALDO_NTM_HORIZONTAL;
        header->mapper_implemented = true;
    } else {
        if (!(self = malloc(sizeof *self))) return ALDO_CART_ERR_ERNO;

        *self = (struct ines_mapper){
            .vtable = {
                .extends = {.mbus_connect = ines_unimplemented_mbus_connect},
                .vbus_connect = ines_unimplemented_vbus_connect,
                .vbus_disconnect = clear_chr_device,
            },
        };
        header->mapper_implemented = false;
    }
    auto base = &self->vtable.extends;
    base->dtor = ines_dtor;
    base->prgrom = ines_prgrom;
    base->mbus_disconnect = clear_prg_device;
    if (header->chr_blocks > 0) {
        self->vtable.chrrom = ines_chrrom;
    }
    self->ptstale = true;
    self->id = header->mapper_id;

    int err;
    if (header->trainer) {
        // skip 512 bytes of trainer data
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
    if (err < 0) goto cleanup;

    if (header->chr_blocks == 0) {
        // TODO: this size is controlled by the mapper in many cases
        if (!(self->chr = calloc(ALDO_MEMBLOCK_8KB, sizeof *self->chr))) {
            err = ALDO_CART_ERR_ERNO;
            goto cleanup;
        }
        self->chrram = true;
    } else {
        err = load_blocks(&self->chr, header->chr_blocks * ALDO_MEMBLOCK_8KB, f);
    }

cleanup:
    if (err == 0) {
        *m = (struct aldo_mapper *)self;
    } else {
        base->dtor((struct aldo_mapper *)self);
    }
    return err;
}
