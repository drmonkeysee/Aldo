//
//  cart.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#include "cart.h"

#include "mappers.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// X(symbol, name)
#define CART_FORMAT_X \
X(CRTF_RAW, "Raw ROM Image?") \
X(CRTF_ALDO, "Aldo") \
X(CRTF_INES, "iNES") \
X(CRTF_NES20, "NES 2.0") \
X(CRTF_NSF, "NES Sound Format")

static const char
    *const restrict NesMagic = "NES\x1a",
    *const restrict NsfMagic = "NESM\x1a";

enum cartformat {
#define X(s, n) s,
    CART_FORMAT_X
#undef X
};

struct cartridge {
    struct mapper *mapper;
    enum cartformat format;
    union {
        struct ines_header ines_hdr;
    };
};

static int detect_format(struct cartridge *self, FILE *f)
{
    // NOTE: grab first 8 bytes as a string to check file format
    char format[9];

    if (!fgets(format, sizeof format, f)) {
        if (feof(f)) return CART_ERR_EOF;
        if (ferror(f)) return CART_ERR_IO;
        return CART_ERR_UNKNOWN;
    }

    if (strncmp(NsfMagic, format, strlen(NsfMagic)) == 0) {
        self->format = CRTF_NSF;
    } else if (strncmp(NesMagic, format, strlen(NesMagic)) == 0) {
        // NOTE: NES 2.0 byte 7 matches pattern 0bxxxx10xx
        self->format = ((unsigned char)format[7] & 0xc) == 0x8
                        ? CRTF_NES20
                        : CRTF_INES;
    } else {
        self->format = CRTF_RAW;
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

    self->ines_hdr.prg_chunks = header[4];
    self->ines_hdr.chr_chunks = header[5];
    self->ines_hdr.wram = header[6] & 0x2;

    // NOTE: mapper may override these two fields
    self->ines_hdr.mirror = header[6] & 0x8
                                ? NTM_4SCREEN
                                : (header[6] & 0x1
                                   ? NTM_VERTICAL
                                   : NTM_HORIZONTAL);
    self->ines_hdr.mapper_controlled = false;

    self->ines_hdr.trainer = header[6] & 0x4;
    self->ines_hdr.mapper_id = (header[6] >> 4) | (header[7] & 0xf0);
    self->ines_hdr.wram_chunks = header[8];
    self->ines_hdr.bus_conflicts = header[10] & 0x20;

    const int err = mapper_ines_create(&self->mapper, &self->ines_hdr, f);
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

static const char *format_name(enum cartformat f)
{
    switch (f) {
#define X(s, n) case s: return n;
        CART_FORMAT_X
#undef X
    default:
        assert(((void)"INVALID CART FORMAT", false));
        break;
    }
}

static const char *mirror_name(enum nt_mirroring m)
{
    switch (m) {
#define X(s, n) case s: return n;
        CART_INES_NTMIRROR_X
#undef X
    default:
        assert(((void)"INVALID INES NT MIRROR", false));
        break;
    }
}

static uint8_t mapper_id(const struct cartridge *self)
{
    return self->format == CRTF_INES ? self->ines_hdr.mapper_id : 0;
}

static void hr(FILE *f)
{
    fputs("-----------------------\n", f);
}

static const char *boolstr(bool value)
{
    return value ? "yes" : "no";
}

static void write_ines_info(const struct cartridge *self, FILE *f,
                            bool verbose)
{
    static const char
        *const restrict fullsize = " x 16KB",
        *const restrict halfsize = " x 8KB",
        *const restrict wramlbl = "WRAM\t\t: ",
        *const restrict chrromlbl = "CHR ROM\t\t: ",
        *const restrict chrramlbl = "CHR RAM\t\t: ";

    fprintf(f, "Mapper\t\t: %03u%s", self->ines_hdr.mapper_id,
            self->ines_hdr.mapper_implemented ? "" : " (Not Implemented)");
    if (verbose) {
        // TODO: add board names
        fputs(" (<Board Names>)\n", f);
        hr(f);
    } else {
        fputc('\n', f);
    }

    fprintf(f, "PRG ROM\t\t: %u%s\n", self->ines_hdr.prg_chunks,
            verbose ? fullsize : "");
    if (self->ines_hdr.wram) {
        fprintf(f, "%s%u%s\n", wramlbl,
                self->ines_hdr.wram_chunks == 0
                    ? 1u
                    : self->ines_hdr.wram_chunks,
                verbose ? halfsize : "");
    } else if (verbose) {
        fprintf(f, "%sno\n", wramlbl);
    }

    if (self->ines_hdr.chr_chunks == 0) {
        if (verbose) {
            fprintf(f, "%sno\n", chrromlbl);
        }
        fprintf(f, "%s1%s\n", chrramlbl, verbose ? halfsize : "");
    } else {
        fprintf(f, "%s%u%s\n", chrromlbl, self->ines_hdr.chr_chunks,
                verbose ? halfsize : "");
        if (verbose) {
            fprintf(f, "%sno\n", chrramlbl);
        }
    }
    fprintf(f, "NT-Mirroring\t: %s\n", mirror_name(self->ines_hdr.mirror));
    if (verbose || self->ines_hdr.mapper_controlled) {
        fprintf(f, "Mapper-Ctrl\t: %s\n",
                boolstr(self->ines_hdr.mapper_controlled));
    }

    if (verbose) {
        hr(f);
    }
    if (verbose || self->ines_hdr.trainer) {
        fprintf(f, "Trainer\t\t: %s\n", boolstr(self->ines_hdr.trainer));
    }
    if (verbose || self->ines_hdr.bus_conflicts) {
        fprintf(f, "Bus Conflicts\t: %s\n",
                boolstr(self->ines_hdr.bus_conflicts));
    }
}

static void write_raw_info(FILE *f)
{
    // TODO: assume 32KB size for now
    fputs("PRG Size\t: 32KB\n", f);
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

int cart_create(cart **c, FILE *f)
{
    assert(c != NULL);
    assert(f != NULL);

    struct cartridge *self = malloc(sizeof *self);

    int err = detect_format(self, f);
    if (err == 0) {
        switch (self->format) {
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
        self = NULL;
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

struct bankview cart_prgbank(cart *self, size_t i)
{
    assert(self != NULL);
    assert(self->mapper != NULL);

    struct bankview bv = {.bank = i};
    bv.size = self->mapper->prgbank(self->mapper, i, &bv.mem);
    return bv;
}

struct bankview cart_chrbank(cart *self, size_t i)
{
    assert(self != NULL);
    assert(self->mapper != NULL);

    struct bankview bv = {.bank = i};
    if (self->mapper->chrbank) {
        bv.size = self->mapper->chrbank(self->mapper, i, &bv.mem);
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

void cart_write_info(cart *self, FILE *f, bool verbose)
{
    assert(self != NULL);
    assert(f != NULL);

    fprintf(f, "Format\t\t: %s\n", format_name(self->format));
    if (verbose) {
        hr(f);
    }
    switch (self->format) {
    case CRTF_INES:
        write_ines_info(self, f, verbose);
        break;
    default:
        write_raw_info(f);
        break;
    }
}

void cart_write_dis_header(cart *self, FILE *f)
{
    char fmtd[CART_FMT_SIZE];
    const int result = cart_fmtdescription(self->format, mapper_id(self),
                                           fmtd);
    fputs(result > 0 ? fmtd : "Invalid Format", f);
    fputs("\n\nDisassembly of PRG Banks\n", f);
    if (self->format != CRTF_ALDO) {
        fputs("(NOTE: approximate for non-Aldo formats)\n", f);
    }
}

int cart_fmtdescription(int format, uint8_t mapid,
                        char buf[restrict static CART_FMT_SIZE])
{
    assert(buf != NULL);

    int count, total;
    total = count = sprintf(buf, "%s", format_name(format));
    if (count < 0) return CART_ERR_FMT;

    if (format == CRTF_INES) {
        count = sprintf(buf + total, " (%03d)", mapid);
        if (count < 0) return CART_ERR_FMT;
        total += count;
    }

    assert(total < CART_FMT_SIZE);
    return total;
}

void cart_snapshot(cart *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->cart.format = self->format;
    snapshot->cart.mapid = mapper_id(self);
}
