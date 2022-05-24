//
//  mappers.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#include "mappers.h"

#include "bytes.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct raw_mapper {
    struct mapper vtable;
    uint8_t *rom;
};

struct ines_mapper {
    struct mapper vtable;
    uint8_t *prg, *chr, *wram, id;
    bool chrram;
};

struct ines_000_mapper {
    struct ines_mapper super;
    size_t bankcount;
};

static int load_chunks(uint8_t *restrict *mem, size_t size, FILE *f)
{
    size_t items_expected, items_read = 0, item_size;

    *mem = calloc(size, sizeof **mem);

    item_size = sizeof **mem;
    items_expected = size;

    while (items_read < items_expected) {
	items_read += fread(mem[item_size * items_read], item_size,
			    items_expected - items_read, f);

	if (feof(f)) return CART_ERR_EOF;
	if (ferror(f)) return CART_ERR_IO;
    }

    return 0;
}

//
// Common Implementation
//

static void clear_bus_device(const struct mapper *self, bus *b, uint16_t addr)
{
    (void)self;
    bus_set(b, addr, (struct busdevice){0});
}

//
// Raw ROM Image Implementation
//

static bool raw_read(const void *restrict ctx, uint16_t addr,
                     uint8_t *restrict d)
{
    // TODO: will raw ever ask for < $8000?
    if (addr < MEMBLOCK_32KB) return false;
    *d = ((const uint8_t *)ctx)[addr & ADDRMASK_32KB];
    return true;
}

static size_t raw_dma(const void *restrict ctx, uint16_t addr, size_t count,
                      uint8_t dest[restrict count])
{
    // TODO: will raw ever ask for < $8000?
    if (addr < MEMBLOCK_32KB) return 0;
    return bytecopy_bank(ctx, BITWIDTH_32KB, addr, count, dest);
}

static void raw_dtor(struct mapper *self)
{
    assert(self != NULL);

    struct raw_mapper *const m = (struct raw_mapper *)self;
    free(m->rom);
    free(m);
}

static size_t raw_prgbank(const struct mapper *self, size_t i,
                          const uint8_t **mem)
{
    assert(self != NULL);
    assert(mem != NULL);

    if (i > 0) {
        *mem = NULL;
        return 0;
    }

    *mem = ((const struct raw_mapper *)self)->rom;
    return MEMBLOCK_32KB;
}

static bool raw_cpu_connect(struct mapper *self, bus *b, uint16_t addr)
{
    return bus_set(b, addr, (struct busdevice){
        .read = raw_read,
        .dma = raw_dma,
        .ctx = ((struct raw_mapper *)self)->rom,
    });
}

//
// iNES Implementation
//

static void ines_dtor(struct mapper *self)
{
    assert(self != NULL);

    struct ines_mapper *const m = (struct ines_mapper *)self;
    free(m->chr);
    free(m->prg);
    free(m->wram);
    free(m);
}

static size_t ines_unimplemented_prgbank(const struct mapper *self, size_t i,
                                         const uint8_t **mem)
{
    assert(self != NULL);
    assert(mem != NULL);

    (void)self, (void)i;
    *mem = NULL;
    return 0;
}

static bool ines_unimplemented_cpu_connect(struct mapper *self, bus *b,
                                           uint16_t addr)
{
    (void)self;
    return bus_set(b, addr, (struct busdevice){0});
}

//
// iNES 000
//

static bool ines_000_read(const void *restrict ctx, uint16_t addr,
                          uint8_t *restrict d)
{
    // TODO: no wram support, did 000 ever have wram?
    if (addr < MEMBLOCK_32KB) return false;
    const struct ines_000_mapper *const m = ctx;
    const uint16_t mask = m->bankcount == 2 ? ADDRMASK_32KB : ADDRMASK_16KB;
    *d = m->super.prg[addr & mask];
    return true;
}

static size_t ines_000_dma(const void *restrict ctx, uint16_t addr,
                           size_t count, uint8_t dest[restrict count])
{
    // TODO: no wram support, did 000 ever have wram?
    if (addr < MEMBLOCK_32KB) return 0;
    const struct ines_000_mapper *const m = ctx;
    return m->bankcount == 2
        ? bytecopy_bank(m->super.prg, BITWIDTH_32KB, addr, count, dest)
        : bytecopy_bankmirrored(m->super.prg, BITWIDTH_16KB, addr,
                                BITWIDTH_64KB, count, dest);
}

static size_t ines_000_prgbank(const struct mapper *self, size_t i,
                               const uint8_t **mem)
{
    assert(self != NULL);
    assert(mem != NULL);

    const struct ines_000_mapper
        *const m = (const struct ines_000_mapper *)self;

    if (i >= m->bankcount) {
        *mem = NULL;
        return 0;
    }

    *mem = m->super.prg + (i * MEMBLOCK_16KB);
    return MEMBLOCK_16KB;
}

static size_t ines_000_chrbank(const struct mapper *self, size_t i,
                               const uint8_t **mem)
{
    assert(self != NULL);
    assert(mem != NULL);

    const struct ines_000_mapper
        *const m = (const struct ines_000_mapper *)self;

    if (m->super.chrram || i > 0) {
        *mem = NULL;
        return 0;
    }

    *mem = m->super.chr + (i * MEMBLOCK_8KB);
    return MEMBLOCK_8KB;
}

static bool ines_000_cpu_connect(struct mapper *self, bus *b, uint16_t addr)
{
    return bus_set(b, addr, (struct busdevice){
        .read = ines_000_read,
        .dma = ines_000_dma,
        .ctx = self,
    });
}

//
// Public Interface
//

int mapper_raw_create(struct mapper **m, FILE *f)
{
    assert(m != NULL);
    assert(f != NULL);

    struct raw_mapper *self = malloc(sizeof *self);
    *self = (struct raw_mapper){
        .vtable = {
            .dtor = raw_dtor,
            .prgbank = raw_prgbank,
            .cpu_connect = raw_cpu_connect,
            .cpu_disconnect = clear_bus_device,
        },
    };

    // TODO: assume a 32KB ROM file (can i do mirroring later?)
    const int err = load_chunks(&self->rom, MEMBLOCK_32KB, f);
    if (err == 0) {
        *m = (struct mapper *)self;
        return 0;
    } else {
        self->vtable.dtor((struct mapper *)self);
        self = NULL;
    }
    return err;
}

int mapper_ines_create(struct mapper **m, struct ines_header *header, FILE *f)
{
    assert(m != NULL);
    assert(header != NULL);
    assert(f != NULL);

    // NOTE: assume implemented, clear flag in default case
    header->mapper_implemented = true;
    struct ines_mapper *self;
    switch (header->mapper_id) {
    case 0:
        self = malloc(sizeof(struct ines_000_mapper));
        *self = (struct ines_mapper){
            .vtable = {
                ines_dtor,
                ines_000_prgbank,
                ines_000_cpu_connect,
                clear_bus_device,

                ines_000_chrbank,
            },
        };
        assert(header->prg_chunks <= 2);
        ((struct ines_000_mapper *)self)->bankcount = header->prg_chunks;
        break;
    default:
        self = malloc(sizeof *self);
        *self = (struct ines_mapper){
            .vtable = {
                .dtor = ines_dtor,
                .prgbank = ines_unimplemented_prgbank,
                .cpu_connect = ines_unimplemented_cpu_connect,
                .cpu_disconnect = clear_bus_device,
            },
        };
        header->mapper_implemented = false;
        break;
    }
    self->id = header->mapper_id;

    int err;
    if (header->trainer) {
        // NOTE: skip 512 bytes of trainer data
        err = fseek(f, 512, SEEK_CUR);
        if (err != 0) return err;
    }

    if (header->wram) {
        const size_t sz = (header->wram_chunks == 0
                           ? 1
                           : header->wram_chunks) * MEMBLOCK_8KB;
        self->wram = calloc(sz, sizeof *self->wram);
    }

    err = load_chunks(&self->prg, header->prg_chunks * MEMBLOCK_16KB, f);
    if (err != 0) return err;

    if (header->chr_chunks == 0) {
        // TODO: this size is controlled by the mapper in many cases
        self->chr = calloc(MEMBLOCK_8KB, sizeof *self->chr);
        self->chrram = true;
    } else {
        err = load_chunks(&self->chr, header->chr_chunks * MEMBLOCK_8KB, f);
    }

    if (err == 0) {
        *m = (struct mapper *)self;
    } else {
        self->vtable.dtor((struct mapper *)self);
        self = NULL;
    }
    return err;
}
