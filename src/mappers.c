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

static int load_blocks(uint8_t *restrict *mem, size_t size, FILE *f)
{
    *mem = calloc(size, sizeof **mem);
    fread(*mem, sizeof **mem, size, f);
    if (feof(f)) return CART_ERR_EOF;
    if (ferror(f)) return CART_ERR_IO;
    return 0;
}

//
// MARK: - Common Implementation
//

static void clear_bus_device(const struct mapper *self, bus *b, uint16_t addr)
{
    (void)self;
    bus_clear(b, addr);
}

//
// MARK: - Raw ROM Image Implementation
//

static bool raw_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ADDRMASK_32KB);

    *d = ((const uint8_t *)ctx)[addr & ADDRMASK_32KB];
    return true;
}

static size_t raw_copy(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ADDRMASK_32KB);

    return bytecopy_bank(ctx, BITWIDTH_32KB, addr, count, dest);
}

static void raw_dtor(struct mapper *self)
{
    assert(self != NULL);

    struct raw_mapper *m = (struct raw_mapper *)self;
    free(m->rom);
    free(m);
}

static const uint8_t *raw_prgrom(const struct mapper *self)
{
    assert(self != NULL);

    return ((const struct raw_mapper *)self)->rom;
}

static bool raw_mbus_connect(struct mapper *self, bus *b, uint16_t addr)
{
    return bus_set(b, addr, (struct busdevice){
        .read = raw_read,
        .copy = raw_copy,
        .ctx = ((struct raw_mapper *)self)->rom,
    });
}

//
// MARK: - iNES Implementation
//

static void ines_dtor(struct mapper *self)
{
    assert(self != NULL);

    struct ines_mapper *m = (struct ines_mapper *)self;
    free(m->chr);
    free(m->prg);
    free(m->wram);
    free(m);
}

static const uint8_t *ines_prgrom(const struct mapper *self)
{
    assert(self != NULL);

    return ((const struct ines_mapper *)self)->prg;
}

static const uint8_t *ines_chrrom(const struct mapper *self)
{
    assert(self != NULL);

    return ((const struct ines_mapper *)self)->chr;
}

static bool ines_unimplemented_mbus_connect(struct mapper *self, bus *b,
                                            uint16_t addr)
{
    (void)self;
    return bus_clear(b, addr);
}

//
// MARK: - iNES 000
//

static bool ines_000_read(void *restrict ctx, uint16_t addr,
                          uint8_t *restrict d)
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ADDRMASK_32KB);

    const struct ines_000_mapper *m = ctx;
    uint16_t mask = m->bankcount == 2 ? ADDRMASK_32KB : ADDRMASK_16KB;
    *d = m->super.prg[addr & mask];
    return true;
}

static size_t ines_000_copy(const void *restrict ctx, uint16_t addr,
                            size_t count, uint8_t dest[restrict count])
{
    // NOTE: addr=[$8000-$FFFF]
    assert(addr > ADDRMASK_32KB);

    const struct ines_000_mapper *m = ctx;
    uint16_t width = m->bankcount == 2 ? BITWIDTH_32KB : BITWIDTH_16KB;
    return bytecopy_bank(m->super.prg, width, addr, count, dest);
}

static bool ines_000_mbus_connect(struct mapper *self, bus *b, uint16_t addr)
{
    return bus_set(b, addr, (struct busdevice){
        .read = ines_000_read,
        .copy = ines_000_copy,
        .ctx = self,
    });
}

//
// MARK: - Public Interface
//

int mapper_raw_create(struct mapper **m, FILE *f)
{
    assert(m != NULL);
    assert(f != NULL);

    struct raw_mapper *self = malloc(sizeof *self);
    *self = (struct raw_mapper){
        .vtable = {
            .dtor = raw_dtor,
            .prgrom = raw_prgrom,
            .mbus_connect = raw_mbus_connect,
            .mbus_disconnect = clear_bus_device,
        },
    };

    // TODO: assume a 32KB ROM file (can i do mirroring later?)
    int err = load_blocks(&self->rom, MEMBLOCK_32KB, f);
    if (err == 0) {
        *m = (struct mapper *)self;
        return 0;
    } else {
        self->vtable.dtor((struct mapper *)self);
    }
    return err;
}

int mapper_ines_create(struct mapper **m, struct ines_header *header, FILE *f)
{
    assert(m != NULL);
    assert(header != NULL);
    assert(f != NULL);

    struct ines_mapper *self;
    switch (header->mapper_id) {
    case 0:
        self = malloc(sizeof(struct ines_000_mapper));
        *self = (struct ines_mapper){
            .vtable = {
                .mbus_connect = ines_000_mbus_connect,
            },
        };
        assert(header->prg_blocks <= 2);
        ((struct ines_000_mapper *)self)->bankcount = header->prg_blocks;
        header->mapper_implemented = true;
        break;
    default:
        self = malloc(sizeof *self);
        *self = (struct ines_mapper){
            .vtable = {
                .mbus_connect = ines_unimplemented_mbus_connect,
            },
        };
        header->mapper_implemented = false;
        break;
    }
    self->vtable.dtor = ines_dtor;
    self->vtable.prgrom = ines_prgrom;
    self->vtable.mbus_disconnect = clear_bus_device;
    if (header->chr_blocks > 0) {
        self->vtable.chrrom = ines_chrrom;
    }
    self->id = header->mapper_id;

    int err;
    if (header->trainer) {
        // NOTE: skip 512 bytes of trainer data
        err = fseek(f, 512, SEEK_CUR);
        if (err != 0) return err;
    }

    if (header->wram) {
        size_t sz = (header->wram_blocks == 0
                     ? 1
                     : header->wram_blocks) * MEMBLOCK_8KB;
        self->wram = calloc(sz, sizeof *self->wram);
    }

    err = load_blocks(&self->prg, header->prg_blocks * MEMBLOCK_16KB, f);
    if (err != 0) return err;

    if (header->chr_blocks == 0) {
        // TODO: this size is controlled by the mapper in many cases
        self->chr = calloc(MEMBLOCK_8KB, sizeof *self->chr);
        self->chrram = true;
    } else {
        err = load_blocks(&self->chr, header->chr_blocks * MEMBLOCK_8KB, f);
    }

    if (err == 0) {
        *m = (struct mapper *)self;
    } else {
        self->vtable.dtor((struct mapper *)self);
    }
    return err;
}
