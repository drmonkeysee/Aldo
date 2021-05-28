//
//  mappers.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#include "mappers.h"

#include "traits.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// NOTE: cart file data is packed into 8 or 16 KB chunks
static const size_t Chunk = 0x2000,
                    DChunk = Chunk * 2;

struct raw_mapper {
    struct mapper vtable;
    uint8_t *rom;
};

struct ines_mapper {
    struct mapper vtable;
    uint8_t *prg, *chr, *wram, id;
};

static int load_chunks(uint8_t **mem, size_t size, FILE *f)
{
    *mem = calloc(size, sizeof **mem);
    fread(*mem, sizeof **mem, size, f);
    if (feof(f)) return CART_ERR_EOF;
    if (ferror(f)) return CART_ERR_IO;
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
    *d = ((const uint8_t *)ctx)[addr & CpuRomAddrMask];
    return true;
}

static size_t raw_dma(const void *restrict ctx, uint16_t addr,
                      uint8_t *restrict dest, size_t count)
{
    const uint16_t bankstart = addr & CpuRomAddrMask;
    const size_t bankcount = NES_ROM_SIZE - bankstart,
                 bytecount = count > bankcount ? bankcount : count;
    const uint8_t *rom = ctx;
    memcpy(dest, rom + bankstart, bytecount * sizeof *dest);
    return bytecount;
}

static void raw_dtor(struct mapper *self)
{
    assert(self != NULL);

    struct raw_mapper *const m = (struct raw_mapper *)self;
    free(m->rom);
    free(m);
}

static uint16_t raw_prgbank(const struct mapper *self, size_t i,
                            const uint8_t *restrict *mem)
{
    assert(self != NULL);
    assert(mem != NULL);

    if (i > 0) {
        *mem = NULL;
        return 0;
    }

    *mem = ((struct raw_mapper *)self)->rom;
    return NES_ROM_SIZE;
}

static bool raw_cpu_connect(struct mapper *self, bus *b, uint16_t addr)
{
    return bus_set(b, addr, (struct busdevice){
        .read = raw_read,
        .dma = raw_dma,
        .ctx = ((const struct raw_mapper *)self)->rom,
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

static bool ines_unimplemented_cpu_connect(struct mapper *self, bus *b,
                                           uint16_t addr)
{
    (void)self;
    return bus_set(b, addr, (struct busdevice){0});
}

static uint16_t ines_unimplemented_prgbank(const struct mapper *self, size_t i,
                                           const uint8_t *restrict *mem)
{
    assert(self != NULL);
    assert(mem != NULL);

    (void)i;
    *mem = NULL;
    return 0;
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
            raw_dtor,
            raw_prgbank,
            raw_cpu_connect,
            clear_bus_device,
        },
    };

    // TODO: assume a 32KB ROM file (can i do mirroring later?)
    const int err = load_chunks(&self->rom, 2 * DChunk, f);
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

    // TODO: create specific mapper based on ID
    struct ines_mapper *self = malloc(sizeof *self);
    *self = (struct ines_mapper){
        .vtable = {
            ines_dtor,
            ines_unimplemented_prgbank,
            ines_unimplemented_cpu_connect,
            clear_bus_device,
        },
        .id = header->mapper_id,
    };
    // TODO: add mapper support
    header->mapper_implemented = false;

    int err;
    if (header->trainer) {
        // NOTE: skip 512 bytes of trainer data
        err = fseek(f, 512, SEEK_CUR);
        if (err != 0) return err;
    }

    if (header->wram) {
        const size_t sz = (header->wram_chunks == 0
                           ? 1
                           : header->wram_chunks) * Chunk;
        self->wram = calloc(sz, sizeof *self->wram);
    }

    err = load_chunks(&self->prg, header->prg_chunks * DChunk, f);
    if (err != 0) return err;

    if (header->chr_chunks == 0) {
        self->chr = calloc(Chunk, sizeof *self->chr);
    } else {
        err = load_chunks(&self->chr, header->chr_chunks * Chunk, f);
    }

    if (err == 0) {
        *m = (struct mapper *)self;
    } else {
        self->vtable.dtor((struct mapper *)self);
        self = NULL;
    }
    return err;
}
