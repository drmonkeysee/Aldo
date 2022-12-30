//
//  cart.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#include "cart.h"

#include "bytes.h"
#include "mappers.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct cartridge {
    struct mapper *mapper;
    struct cartinfo info;
};

static int detect_format(struct cartridge *self, FILE *f)
{
    static const char
        *const restrict nesmagic = "NES\x1a",
        *const restrict nsfmagic = "NESM\x1a";

    // NOTE: grab first 8 bytes as a string to check file format
    char format[9];

    if (!fgets(format, sizeof format, f)) {
        if (feof(f)) return CART_ERR_EOF;
        if (ferror(f)) return CART_ERR_IO;
        return CART_ERR_UNKNOWN;
    }

    if (strncmp(nsfmagic, format, strlen(nsfmagic)) == 0) {
        self->info.format = CRTF_NSF;
    } else if (strncmp(nesmagic, format, strlen(nesmagic)) == 0) {
        // NOTE: NES 2.0 byte 7 matches pattern 0bxxxx10xx
        self->info.format = ((unsigned char)format[7] & 0xc) == 0x8
                                ? CRTF_NES20
                                : CRTF_INES;
    } else {
        self->info.format = CRTF_RAW;
    }

    // NOTE: reset back to beginning of file to fully parse detected format
    return fseek(f, 0, SEEK_SET) == 0 ? 0 : CART_ERR_IO;
}

static int parse_ines(struct cartridge *self, FILE *f)
{
    unsigned char header[16];

    fread(header, sizeof header[0], sizeof header, f);
    if (feof(f)) return CART_ERR_EOF;
    if (ferror(f)) return CART_ERR_IO;

    // NOTE: if last 4 bytes of header aren't 0 this is a very old format
    uint32_t tail;
    memcpy(&tail, header + 12, sizeof tail);
    if (tail != 0) return CART_ERR_OBSOLETE;

    struct cartinfo *const info = &self->info;
    info->ines_hdr.prg_blocks = header[4];
    info->ines_hdr.chr_blocks = header[5];
    info->ines_hdr.wram = header[6] & 0x2;

    // NOTE: mapper may override these two fields
    info->ines_hdr.mirror = header[6] & 0x8
                                ? NTM_4SCREEN
                                : (header[6] & 0x1
                                   ? NTM_VERTICAL
                                   : NTM_HORIZONTAL);
    info->ines_hdr.mapper_controlled = false;

    info->ines_hdr.trainer = header[6] & 0x4;
    info->ines_hdr.mapper_id = (uint8_t)((header[6] >> 4)
                                         | (header[7] & 0xf0));
    info->ines_hdr.wram_blocks = header[8];
    info->ines_hdr.bus_conflicts = header[10] & 0x20;

    const int err = mapper_ines_create(&self->mapper, &info->ines_hdr, f);
    if (err == 0) {
        // TODO: we've found a ROM with extra bytes after CHR data
        assert(fgetc(f) == EOF && feof(f));
    }
    return err;
}

// NOTE: a raw ROM image is just a stream of bytes and has no identifying
// header; if format cannot be determined, this is the default.
static int parse_raw(struct cartridge *self, FILE *f)
{
    int err = mapper_raw_create(&self->mapper, f);
    // NOTE: ROM file is too big for prg address space (no bank-switching)
    if (err == 0 && !(fgetc(f) == EOF && feof(f))) {
        err = CART_ERR_IMG_SIZE;
    }
    return err;
}

static void hr(FILE *f)
{
    fputs("-----------------------\n", f);
}

static const char *boolstr(bool value)
{
    return value ? "yes" : "no";
}

static void write_ines_info(const struct cartinfo *info, FILE *f, bool verbose)
{
    static const char
        *const restrict fullsize = " x 16KB",
        *const restrict halfsize = " x 8KB",
        *const restrict wramlbl = "WRAM\t\t: ",
        *const restrict chrromlbl = "CHR ROM\t\t: ",
        *const restrict chrramlbl = "CHR RAM\t\t: ";

    fprintf(f, "Mapper\t\t: %03u%s\n", info->ines_hdr.mapper_id,
            info->ines_hdr.mapper_implemented ? "" : " (Not Implemented)");
    if (verbose) {
        // TODO: add board names
        fputs("Boards\t\t: <Board Names>\n", f);
        hr(f);
    }

    fprintf(f, "PRG ROM\t\t: %u%s\n", info->ines_hdr.prg_blocks,
            verbose ? fullsize : "");
    if (info->ines_hdr.wram) {
        fprintf(f, "%s%u%s\n", wramlbl,
                info->ines_hdr.wram_blocks > 0
                    ? info->ines_hdr.wram_blocks
                    : 1u,
                verbose ? halfsize : "");
    } else if (verbose) {
        fprintf(f, "%sno\n", wramlbl);
    }

    if (info->ines_hdr.chr_blocks > 0) {
        fprintf(f, "%s%u%s\n", chrromlbl, info->ines_hdr.chr_blocks,
                verbose ? halfsize : "");
        if (verbose) {
            fprintf(f, "%sno\n", chrramlbl);
        }
    } else {
        if (verbose) {
            fprintf(f, "%sno\n", chrromlbl);
        }
        fprintf(f, "%s1%s\n", chrramlbl, verbose ? halfsize : "");
    }
    fprintf(f, "NT-Mirroring\t: %s\n", cart_mirrorname(info->ines_hdr.mirror));
    if (verbose || info->ines_hdr.mapper_controlled) {
        fprintf(f, "Mapper-Ctrl\t: %s\n",
                boolstr(info->ines_hdr.mapper_controlled));
    }

    if (verbose) {
        hr(f);
    }
    if (verbose || info->ines_hdr.trainer) {
        fprintf(f, "Trainer\t\t: %s\n", boolstr(info->ines_hdr.trainer));
    }
    if (verbose || info->ines_hdr.bus_conflicts) {
        fprintf(f, "Bus Conflicts\t: %s\n",
                boolstr(info->ines_hdr.bus_conflicts));
    }
}

static void write_raw_info(FILE *f)
{
    // TODO: assume 32KB size for now
    fputs("PRG ROM\t: 1 x 32KB\n", f);
}

//
// Public Interface
//

const char *cart_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case s: return e;
        CART_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

const char *cart_formatname(enum cartformat format)
{
    switch (format) {
#define X(s, n) case s: return n;
        CART_FORMAT_X
#undef X
    default:
        return "UNKNOWN CART FORMAT";
    }
}

const char *cart_mirrorname(enum nt_mirroring mirror)
{
    switch (mirror) {
#define X(s, n) case s: return n;
        CART_INES_NTMIRROR_X
#undef X
    default:
        return "UNKNOWN INES NT MIRROR";
    }
}

int cart_create(cart **c, FILE *f)
{
    assert(c != NULL);
    assert(f != NULL);

    struct cartridge *const self = malloc(sizeof *self);

    int err = detect_format(self, f);
    if (err == 0) {
        switch (self->info.format) {
        case CRTF_INES:
            err = parse_ines(self, f);
            break;
        case CRTF_ALDO:
        case CRTF_NES20:
        case CRTF_NSF:
            err = CART_ERR_FORMAT;
            break;
        default:
            err = parse_raw(self, f);
            break;
        }
    }

    if (err == 0) {
        *c = self;
    } else {
        cart_free(self);
    }
    return err;
}

void cart_free(cart *self)
{
    assert(self != NULL);

    if (self->mapper) {
        self->mapper->dtor(self->mapper);
    }
    free(self);
}

void cart_getinfo(cart *self, struct cartinfo *info)
{
    assert(self != NULL);
    assert(info != NULL);

    *info = self->info;
}

struct blockview cart_prgblock(cart *self, size_t i)
{
    assert(self != NULL);
    assert(self->mapper != NULL);

    struct blockview bv = {.ord = i};
    const uint8_t *const prg = self->mapper->prgrom(self->mapper);
    switch (self->info.format) {
    case CRTF_INES:
        if (i < self->info.ines_hdr.prg_blocks) {
            bv.size = MEMBLOCK_16KB;
            bv.mem = prg + (i * bv.size);
        }
        break;
    default:
        if (i == 0) {
            bv.mem = prg;
            bv.size = MEMBLOCK_32KB;
        }
        break;
    }
    return bv;
}

struct blockview cart_chrblock(cart *self, size_t i)
{
    assert(self != NULL);
    assert(self->mapper != NULL);

    struct blockview bv = {.ord = i};
    if (!self->mapper->chrrom) return bv;

    // NOTE: only iNES carts have chr rom
    const uint8_t *const chr = self->mapper->chrrom(self->mapper);
    if (i < self->info.ines_hdr.chr_blocks) {
        bv.size = MEMBLOCK_8KB;
        bv.mem = chr + (i * bv.size);
    }
    return bv;
}

int cart_cpu_connect(cart *self, bus *b, uint16_t addr)
{
    assert(self != NULL);
    assert(self->mapper != NULL);
    assert(b != NULL);

    return self->mapper->cpu_connect(self->mapper, b, addr)
            ? 0
            : CART_ERR_ADDR_UNAVAILABLE;
}

void cart_cpu_disconnect(cart *self, bus *b, uint16_t addr)
{
    assert(self != NULL);
    assert(self->mapper != NULL);
    assert(b != NULL);

    self->mapper->cpu_disconnect(self->mapper, b, addr);
}

void cart_write_info(cart *self, const char *restrict name, bool verbose,
                     FILE *f)
{
    assert(self != NULL);
    assert(name != NULL);
    assert(f != NULL);

    fprintf(f, "File\t\t: %s\n", name);
    fprintf(f, "Format\t\t: %s\n", cart_formatname(self->info.format));
    if (verbose) {
        hr(f);
    }
    switch (self->info.format) {
    case CRTF_INES:
        write_ines_info(&self->info, f, verbose);
        break;
    default:
        write_raw_info(f);
        break;
    }
}

void cart_write_dis_header(cart *self, const char *restrict name, FILE *f)
{
    assert(self != NULL);
    assert(name != NULL);
    assert(f != NULL);

    fprintf(f, "%s\n", name);
    char fmtd[CART_FMT_SIZE];
    const int err = cart_format_extname(self, fmtd);
    fputs(err < 0 ? cart_errstr(err) : fmtd, f);
    fputs("\n\nDisassembly of PRG ROM\n", f);
    if (self->info.format != CRTF_ALDO) {
        fputs("(NOTE: approximate for non-Aldo formats)\n", f);
    }
}

int cart_format_extname(cart *self, char buf[restrict static CART_FMT_SIZE])
{
    assert(buf != NULL);

    if (!self) return CART_ERR_NOCART;

    int count, total;
    total = count = sprintf(buf, "%s", cart_formatname(self->info.format));
    if (count < 0) return CART_ERR_FMT;

    if (self->info.format == CRTF_INES) {
        count = sprintf(buf + total, " (%03d)", self->info.ines_hdr.mapper_id);
        if (count < 0) return CART_ERR_FMT;
        total += count;
    }

    assert(total < CART_FMT_SIZE);
    return total;
}
