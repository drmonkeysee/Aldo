//
//  cart.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#include "cart.h"

#include "bus.h"
#include "bytes.h"
#include "mappers.h"
#include "snapshot.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define as_nesmap(cart) ((const struct aldo_nesmapper *)((cart)->mapper))

struct aldo_cartridge {
    struct aldo_mapper *mapper;
    struct aldo_cartinfo info;
};

static int detect_format(struct aldo_cartridge *self, FILE *f)
{
    static const char
        *const restrict nesmagic = "NES\x1a",
        *const restrict nsfmagic = "NESM\x1a";

    // NOTE: grab first 8 bytes as a string to check file format
    char format[9];

    if (!fgets(format, sizeof format, f)) {
        if (feof(f)) return ALDO_CART_ERR_EOF;
        if (ferror(f)) return ALDO_CART_ERR_IO;
        return ALDO_CART_ERR_UNKNOWN;
    }

    if (strncmp(nsfmagic, format, strlen(nsfmagic)) == 0) {
        self->info.format = ALDO_CRTF_NSF;
    } else if (strncmp(nesmagic, format, strlen(nesmagic)) == 0) {
        // NOTE: NES 2.0 byte 7 matches pattern 0bxxxx10xx
        self->info.format = ((unsigned char)format[7] & 0xc) == 0x8
                                ? ALDO_CRTF_NES20
                                : ALDO_CRTF_INES;
    } else {
        self->info.format = ALDO_CRTF_RAW;
    }

    // NOTE: reset back to beginning of file to fully parse detected format
    return fseek(f, 0, SEEK_SET) == 0 ? 0 : ALDO_CART_ERR_IO;
}

static int parse_ines(struct aldo_cartridge *self, FILE *f)
{
    unsigned char header[16];

    if (fread(header, sizeof header[0], sizeof header, f) < sizeof header) {
        if (feof(f)) return ALDO_CART_ERR_EOF;
        if (ferror(f)) return ALDO_CART_ERR_IO;
        return ALDO_CART_ERR_UNKNOWN;
    }

    // NOTE: if last 4 bytes of header aren't 0 this is a very old format
    uint32_t tail;
    memcpy(&tail, header + 12, sizeof tail);
    if (tail != 0) return ALDO_CART_ERR_OBSOLETE;

    struct aldo_cartinfo *info = &self->info;
    info->ines_hdr.prg_blocks = header[4];
    info->ines_hdr.chr_blocks = header[5];
    info->ines_hdr.wram = header[6] & 0x2;

    // NOTE: mapper may override these two fields
    info->ines_hdr.mirror = header[6] & 0x8
                                ? ALDO_NTM_4SCREEN
                                : (header[6] & 0x1
                                   ? ALDO_NTM_VERTICAL
                                   : ALDO_NTM_HORIZONTAL);
    info->ines_hdr.mapper_controlled = false;

    info->ines_hdr.trainer = header[6] & 0x4;
    info->ines_hdr.mapper_id =
        (uint8_t)((header[6] >> 4) | (header[7] & 0xf0));
    info->ines_hdr.wram_blocks = header[8];
    info->ines_hdr.bus_conflicts = header[10] & 0x20;

    int err = aldo_mapper_ines_create(&self->mapper, &info->ines_hdr, f);
    if (err == 0) {
        // TODO: we've found a ROM with extra bytes after CHR data
        assert(fgetc(f) == EOF && feof(f));
    }
    return err;
}

// NOTE: a raw ROM image is just a stream of bytes and has no identifying
// header; if format cannot be determined, this is the default.
static int parse_raw(struct aldo_cartridge *self, FILE *f)
{
    int err = aldo_mapper_raw_create(&self->mapper, f);
    // NOTE: ROM file is too big for prg address space (no bank-switching)
    if (err == 0 && !(fgetc(f) == EOF && feof(f))) {
        err = ALDO_CART_ERR_IMG_SIZE;
    }
    return err;
}

static bool hr(FILE *f)
{
    return fputs("-----------------------\n", f) != EOF;
}

static const char *boolstr(bool value)
{
    return value ? "yes" : "no";
}

static int write_ines_info(const struct aldo_cartinfo *info, FILE *f,
                           bool verbose)
{
    static const char
        *const restrict fullsize = " x 16KB",
        *const restrict halfsize = " x 8KB",
        *const restrict wramlbl = "WRAM\t\t: ",
        *const restrict chrromlbl = "CHR ROM\t\t: ",
        *const restrict chrramlbl = "CHR RAM\t\t: ";


    int err = fprintf(f, "Mapper\t\t: %03u%s\n", info->ines_hdr.mapper_id,
                      info->ines_hdr.mapper_implemented
                          ? ""
                          : " (Not Implemented)");
    if (err < 0) goto io_failure;
    if (verbose) {
        // TODO: add board names
        if (fputs("Boards\t\t: <Board Names>\n", f) == EOF || !hr(f))
            goto io_failure;
    }

    err = fprintf(f, "PRG ROM\t\t: %u%s\n", info->ines_hdr.prg_blocks,
                  verbose ? fullsize : "");
    if (err < 0) goto io_failure;
    if (info->ines_hdr.wram) {
        err = fprintf(f, "%s%u%s\n", wramlbl,
                      info->ines_hdr.wram_blocks > 0
                          ? info->ines_hdr.wram_blocks
                          : 1u,
                      verbose ? halfsize : "");
        if (err < 0) goto io_failure;
    } else if (verbose && fprintf(f, "%sno\n", wramlbl) < 0) {
        goto io_failure;
    }

    if (info->ines_hdr.chr_blocks > 0) {
        err = fprintf(f, "%s%u%s\n", chrromlbl, info->ines_hdr.chr_blocks,
                      verbose ? halfsize : "");
        if (err < 0) goto io_failure;
        if (verbose && fprintf(f, "%sno\n", chrramlbl) < 0) goto io_failure;
    } else {
        if (verbose && fprintf(f, "%sno\n", chrromlbl) < 0) goto io_failure;
        err = fprintf(f, "%s1%s\n", chrramlbl, verbose ? halfsize : "");
        if (err < 0) goto io_failure;
    }
    err = fprintf(f, "NT-Mirroring\t: %s\n",
                  aldo_ntmirror_name(info->ines_hdr.mirror));
    if (err < 0) goto io_failure;
    if (verbose || info->ines_hdr.mapper_controlled) {
        err = fprintf(f, "Mapper-Ctrl\t: %s\n",
                      boolstr(info->ines_hdr.mapper_controlled));
        if (err < 0) goto io_failure;
    }

    if (verbose && !hr(f)) goto io_failure;
    if (verbose || info->ines_hdr.trainer) {
        err = fprintf(f, "Trainer\t\t: %s\n", boolstr(info->ines_hdr.trainer));
        if (err < 0) goto io_failure;
    }
    if (verbose || info->ines_hdr.bus_conflicts) {
        err = fprintf(f, "Bus Conflicts\t: %s\n",
                      boolstr(info->ines_hdr.bus_conflicts));
        if (err < 0) goto io_failure;
    }
    return 0;

io_failure:
    return ALDO_CART_ERR_IO;
}

static int write_raw_info(FILE *f)
{
    // TODO: assume 32KB size for now
    return fputs("PRG ROM\t: 1 x 32KB\n", f) == EOF ? ALDO_CART_ERR_IO : 0;
}

static bool is_nes(const struct aldo_cartridge *self)
{
    return self->info.format == ALDO_CRTF_INES;
}

//
// MARK: - Public Interface
//

const char *aldo_cart_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case ALDO_##s: return e;
        ALDO_CART_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

int aldo_cart_create(aldo_cart **c, FILE *f)
{
    assert(c != NULL);
    assert(f != NULL);

    struct aldo_cartridge *self = malloc(sizeof *self);
    if (!self) return ALDO_CART_ERR_ERNO;

    int err = detect_format(self, f);
    if (err == 0) {
        switch (self->info.format) {
        case ALDO_CRTF_INES:
            err = parse_ines(self, f);
            break;
        case ALDO_CRTF_ALDO:
        case ALDO_CRTF_NES20:
        case ALDO_CRTF_NSF:
            err = ALDO_CART_ERR_FORMAT;
            break;
        default:
            err = parse_raw(self, f);
            break;
        }
    }

    if (err == 0) {
        *c = self;
    } else {
        aldo_cart_free(self);
    }
    return err;
}

void aldo_cart_free(aldo_cart *self)
{
    assert(self != NULL);

    if (self->mapper) {
        self->mapper->dtor(self->mapper);
    }
    free(self);
}

const char *aldo_cart_formatname(enum aldo_cartformat format)
{
    switch (format) {
#define X(s, n) case ALDO_##s: return n;
        ALDO_CART_FORMAT_X
#undef X
    default:
        return "UNKNOWN CART FORMAT";
    }
}

int aldo_cart_format_extname(aldo_cart *self,
                             char buf[restrict static ALDO_CART_FMT_SIZE])
{
    assert(buf != NULL);

    if (!self) return ALDO_CART_ERR_NOCART;

    int count, total;
    total = count = sprintf(buf, "%s",
                            aldo_cart_formatname(self->info.format));
    if (count < 0) return ALDO_CART_ERR_FMT;

    if (is_nes(self)) {
        count = sprintf(buf + total, " (%03d)", self->info.ines_hdr.mapper_id);
        if (count < 0) return ALDO_CART_ERR_FMT;
        total += count;
    }

    assert(total < ALDO_CART_FMT_SIZE);
    return total;
}

int aldo_cart_write_info(aldo_cart *self, const char *restrict name,
                         bool verbose, FILE *f)
{
    assert(self != NULL);
    assert(name != NULL);
    assert(f != NULL);

    int err = fprintf(f, "File\t\t: %s\n", name);
    if (err < 0) return ALDO_CART_ERR_IO;
    err = fprintf(f, "Format\t\t: %s\n",
                  aldo_cart_formatname(self->info.format));
    if (err < 0) return ALDO_CART_ERR_IO;
    if (verbose && !hr(f)) return ALDO_CART_ERR_IO;

    return is_nes(self)
            ? write_ines_info(&self->info, f, verbose)
            : write_raw_info(f);
}

void aldo_cart_getinfo(aldo_cart *self, struct aldo_cartinfo *info)
{
    assert(self != NULL);
    assert(info != NULL);

    *info = self->info;
}

struct aldo_blockview aldo_cart_prgblock(aldo_cart *self, size_t i)
{
    assert(self != NULL);
    assert(self->mapper != NULL);

    struct aldo_blockview bv = {.ord = i};
    const uint8_t *prg = self->mapper->prgrom(self->mapper);
    if (is_nes(self)) {
        if (i < self->info.ines_hdr.prg_blocks) {
            bv.size = ALDO_MEMBLOCK_16KB;
            bv.mem = prg + (i * bv.size);
        }
    } else if (i == 0) {
        bv.mem = prg;
        bv.size = ALDO_MEMBLOCK_32KB;
    }
    return bv;
}

struct aldo_blockview aldo_cart_chrblock(aldo_cart *self, size_t i)
{
    assert(self != NULL);
    assert(self->mapper != NULL);

    struct aldo_blockview bv = {.ord = i};
    if (!is_nes(self)) return bv;

    const uint8_t *chr = as_nesmap(self)->chrrom(self->mapper);
    if (i < self->info.ines_hdr.chr_blocks) {
        bv.size = ALDO_MEMBLOCK_8KB;
        bv.mem = chr + (i * bv.size);
    }
    return bv;
}

//
// MARK: - Internal Interface
//

bool aldo_cart_mbus_connect(aldo_cart *self, aldo_bus *b)
{
    assert(self != NULL);
    assert(self->mapper != NULL);
    assert(b != NULL);

    return self->mapper->mbus_connect(self->mapper, b);
}

void aldo_cart_mbus_disconnect(aldo_cart *self, aldo_bus *b)
{
    assert(self != NULL);
    assert(self->mapper != NULL);
    assert(b != NULL);

    self->mapper->mbus_disconnect(b);
}

bool aldo_cart_vbus_connect(aldo_cart *self, aldo_bus *b)
{
    assert(self != NULL);
    assert(self->mapper != NULL);
    assert(b != NULL);

    if (is_nes(self)) {
        return as_nesmap(self)->vbus_connect(self->mapper, b);
    }
    return false;
}

void aldo_cart_vbus_disconnect(aldo_cart *self, aldo_bus *b)
{
    assert(self != NULL);
    assert(self->mapper != NULL);
    assert(b != NULL);

    if (is_nes(self)) {
        as_nesmap(self)->vbus_disconnect(b);
    }
}

int aldo_cart_write_dis_header(aldo_cart *self, const char *restrict name,
                               FILE *f)
{
    assert(self != NULL);
    assert(name != NULL);
    assert(f != NULL);

    int err = fprintf(f, "%s\n", name);
    if (err < 0) return ALDO_CART_ERR_IO;

    char fmtd[ALDO_CART_FMT_SIZE];
    err = aldo_cart_format_extname(self, fmtd);
    if (fputs(err < 0 ? aldo_cart_errstr(err) : fmtd, f) == EOF)
        return ALDO_CART_ERR_IO;
    if (fputs("\n\nDisassembly of PRG ROM\n", f) == EOF)
        return ALDO_CART_ERR_IO;
    if (self->info.format != ALDO_CRTF_ALDO
        && fputs("(NOTE: approximate for non-Aldo formats)\n", f) == EOF)
        return ALDO_CART_ERR_IO;
    return 0;
}

void aldo_cart_snapshot(aldo_cart *self, struct aldo_snapshot *snp)
{
    assert(self != NULL);
    assert(self->mapper != NULL);
    assert(snp != NULL);

    if (is_nes(self) && as_nesmap(self)->snapshot) {
        as_nesmap(self)->snapshot(self->mapper, snp);
    }
}
