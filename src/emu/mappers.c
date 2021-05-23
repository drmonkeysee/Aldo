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

// NOTE: cart file data is packed into 8 or 16 KB chunks
static const size_t Chunk = 0x2000,
                    DChunk = Chunk * 2;

struct rom_img_mapper {
    struct mapper vtable;
    uint8_t *rom;
};

struct ines_mapper {
    struct mapper vtable;
    uint8_t id, *prg, *chr, *wram;
};

static int load_chunks(uint8_t **mem, size_t size, FILE *f)
{
    *mem = calloc(size, sizeof **mem);
    fread(*mem, sizeof **mem, size, f);
    if (feof(f)) return CART_EOF;
    if (ferror(f)) return CART_IO_ERR;
    return 0;
}

//
// ROM Image Implementation
//

static bool rom_img_read(const void *restrict ctx, uint16_t addr,
                         uint8_t *restrict d)
{
    *d = ((const uint8_t *)ctx)[addr & CpuRomAddrMask];
    return true;
}

static void rom_img_dtor(struct mapper *self)
{
    struct rom_img_mapper *const m = (struct rom_img_mapper *)self;
    free(m->rom);
    free(m);
}

static struct busdevice rom_img_make_cpudevice(const struct mapper *self)
{
    return (struct busdevice){
        .read = rom_img_read,
        .ctx = ((const struct rom_img_mapper *)self)->rom,
    };
}

static uint8_t *rom_img_getprg(const struct mapper *self)
{
    return ((const struct rom_img_mapper *)self)->rom;
}

//
// iNES Implementation
//

static void ines_dtor(struct mapper *self)
{
    struct ines_mapper *const m = (struct ines_mapper *)self;
    free(m->chr);
    free(m->prg);
    free(m->wram);
    free(m);
}

static struct busdevice ines_make_cpudevice(const struct mapper *self)
{
    (void)self;
    return (struct busdevice){0};
}

static uint8_t *ines_getprg(const struct mapper *self)
{
    return ((const struct ines_mapper *)self)->prg;
}

//
// Public Interface
//

int mapper_rom_img_create(struct mapper **m, FILE *f)
{
    assert(m != NULL);
    assert(f != NULL);

    struct rom_img_mapper *self = malloc(sizeof *self);
    *self = (struct rom_img_mapper){
        .vtable = {rom_img_dtor, rom_img_make_cpudevice, rom_img_getprg},
    };

    // TODO: assume a 32KB ROM file
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

int mapper_ines_create(struct mapper **m, const struct ines_header *header,
                       FILE *f)
{
    assert(m != NULL);
    assert(header != NULL);
    assert(f != NULL);

    // TODO: create specific mapper based on ID
    struct ines_mapper *self = malloc(sizeof *self);
    *self = (struct ines_mapper){
        .vtable = {ines_dtor, ines_make_cpudevice, ines_getprg},
        .id = header->mapper_id,
    };

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
