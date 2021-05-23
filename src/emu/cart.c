//
//  cart.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#include "cart.h"

#include "mappers.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// X(symbol, name)
#define CART_FORMAT_X \
X(ROM_IMG, "ROM Image?") \
X(ALDO, "Aldo") \
X(INES, "iNES") \
X(NES20, "NES 2.0") \
X(NSF, "NES Sound Format")

#define CRTF_ENUM(s) CRTF_##s

static const char *restrict const NesFormat = "NES\x1a",
                  *restrict const NsfFormat = "NESM\x1a";

enum cartformat {
#define X(s, n) CRTF_ENUM(s),
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
    const char *const fmtsuccess = fgets(format, sizeof format, f);

    if (!fmtsuccess) {
        if (feof(f)) return CART_EOF;
        if (ferror(f)) return CART_IO_ERR;
        return CART_UNKNOWN_ERR;
    }

    if (strncmp(NsfFormat, format, strlen(NsfFormat)) == 0) {
        self->format = CRTF_NSF;
    } else if (strncmp(NesFormat, format, strlen(NesFormat)) == 0) {
        // NOTE: NES 2.0 byte 7 matches pattern 0bxxxx10xx
        self->format = ((unsigned char)format[7] & 0xc) == 0x8
                       ? CRTF_NES20
                       : CRTF_INES;
    } else {
        self->format = CRTF_ROM_IMG;
    }

    // NOTE: reset back to beginning of file to fully parse detected format
    return fseek(f, 0, SEEK_SET) == 0 ? 0 : CART_IO_ERR;
}

static int parse_ines(struct cartridge *self, FILE *f)
{
    unsigned char header[16];

    fread(header, sizeof header[0], sizeof header, f);
    if (feof(f)) return CART_EOF;
    if (ferror(f)) return CART_IO_ERR;

    // NOTE: if last 4 bytes of header aren't 0 this is a very old format
    uint32_t tail;
    memcpy(&tail, header + 12, sizeof tail);
    if (tail != 0) return CART_OBSOLETE;

    // TODO: finish off setting header fields
    self->ines_hdr.prg_chunks = header[4];
    self->ines_hdr.chr_chunks = header[5];
    self->ines_hdr.wram = header[6] & 0x2;
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

// TODO: load file contents into a single ROM bank and hope for the best
static int parse_rom(struct cartridge *self, FILE *f)
{
    const int err = mapper_rom_img_create(&self->mapper, f);
    if (err == 0 && !(fgetc(f) == EOF && feof(f))) {
        // NOTE: ROM file is too big for prg address space
        return CART_IMG_SIZE;
    }
    return err;
}

static const char *format_name(enum cartformat f)
{
    switch (f) {
#define X(s, n) case CRTF_ENUM(s): return n;
        CART_FORMAT_X
#undef X
    default:
        assert(((void)"BAD CART FORMAT", false));
    }
}

static void hr(FILE *f)
{
    fprintf(f, "-----------------------\n");
}

static void write_ines_info(const struct cartridge *self, FILE *f,
                            bool verbose)
{
    static const char
        *restrict const fullsize = " x 16KB",
        *restrict const halfsize = " x 8KB",
        *restrict const wramlbl = "WRAM\t\t: ",
        *restrict const chrromlbl = "CHR ROM\t\t: ",
        *restrict const chrramlbl = "CHR RAM\t\t: ";

    fprintf(f, "Mapper\t\t: %03u", self->ines_hdr.mapper_id);
    if (verbose) {
        fprintf(f, " (<Board Names>)\n");
        hr(f);
    } else {
        fprintf(f, "\n");
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

    if (verbose) {
        hr(f);
    }
    if (verbose || self->ines_hdr.trainer) {
        fprintf(f, "Trainer\t\t: %s\n", self->ines_hdr.trainer ? "yes" : "no");
    }
    if (verbose || self->ines_hdr.bus_conflicts) {
        fprintf(f, "Bus Conflicts\t: %s\n",
                self->ines_hdr.bus_conflicts ? "yes" : "no");
    }
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
        default:
            err = parse_rom(self, f);
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
    if (self->mapper) {
        self->mapper->dtor(self->mapper);
    }
    free(self);
}

// TODO: get rid of this
uint8_t *cart_prg_bank(cart *self)
{
    assert(self != NULL);

    return self->mapper->getprg(self->mapper);
}

int cart_cpu_connect(cart *self, bus *b, uint16_t addr)
{
    assert(self != NULL);
    assert(b != NULL);

    return bus_set(b, addr, self->mapper->make_cpudevice(self->mapper))
           ? 0
           : CART_ADDR_UNAVAILABLE;
}

void cart_info_write(cart *self, FILE *f, bool verbose)
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
        break;
    }
}

void cart_snapshot(cart *self, struct console_state *snapshot)
{
    assert(self != NULL);

    snapshot->rom = self->mapper->getprg(self->mapper);
}
