//
//  cart.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#include "cart.h"

#include "traits.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// X(symbol, name)
#define CART_FORMAT_X \
X(UNKNOWN, "Unknown") \
X(ALDOBIN, "Aldo Binary") \
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

// A NES game cartridge including PRG and CHR ROM banks,
// WRAM or save-state RAM, memory mapper circuitry, and metadata headers.

// TODO: ignoring following fields for now:
//  - hardcoded mirroring bits, need to know mapper to interpret these
//  - VS/Playchoice system indicator (does anyone care?)
//  - TV System (PAL ROMs don't seem to set the flags so again who cares)
//  - redundant indicators in byte 10
struct ines_header {
    uint8_t prg_banks,      // Count of 16KB PRG ROM banks
            chr_banks,      // Count of 8KB CHR ROM banks (0 indicates CHR RAM)
            prg_rambanks,   // Count of 8KB PRG RAM banks
            mapper_id;      // Mapper ID
    bool prg_ram,           // PRG RAM banks present
         trainer,           // Trainer data present
         bus_conflicts;     // Cart has bus conflicts
};

struct cartridge {
    enum cartformat format;         // Cart File Format
    union {
        struct ines_header ns_hdr;  // iNES Header
    };
    uint8_t prg[ROM_SIZE];  // TODO: assume a single bank of 32KB PRG for now
};

static int detect_format(cart *self, FILE *f)
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
        self->format = CRTF_UNKNOWN;
    }

    // NOTE: reset back to beginning of file to fully parse detected format
    return fseek(f, 0, SEEK_SET) == 0 ? 0 : CART_IO_ERR;
}

static int parse_ines(cart *self, FILE *f)
{
    unsigned char header[16];

    fread(header, sizeof header[0], sizeof header, f);
    if (feof(f)) return CART_EOF;
    if (ferror(f)) return CART_IO_ERR;

    // NOTE: if last 4 bytes of header aren't 0 this is a very old format
    uint32_t tail;
    memcpy(&tail, header + 12, sizeof tail);
    if (tail != 0) return CART_OBSOLETE;

    self->ns_hdr.prg_banks = header[4];
    self->ns_hdr.chr_banks = header[5];
    self->ns_hdr.prg_ram = header[6] & 0x2;
    self->ns_hdr.trainer = header[6] & 0x4;
    self->ns_hdr.mapper_id = (header[6] >> 4) | (header[7] & 0xf0);
    self->ns_hdr.prg_rambanks = header[8];
    self->ns_hdr.bus_conflicts = header[10] & 0x20;

    // TODO: parse ROM/RAM banks

    return 0;
}

// TODO: load file contents into a single ROM bank and hope for the best
static int parse_unknown(cart *self, FILE *f)
{
    const size_t bufsize = sizeof self->prg / sizeof self->prg[0],
                 count = fread(self->prg, sizeof self->prg[0], bufsize, f);

    if (feof(f))  return 0;
    if (ferror(f)) return CART_IO_ERR;
    if (count == bufsize) return CART_PRG_SIZE;
    return CART_UNKNOWN_ERR;
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

static void write_ines_info(cart *self, FILE *f, bool verbose)
{
    static const char
        *restrict const fullsize = " x 16KB",
        *restrict const halfsize = " x 8KB",
        *restrict const prgramlbl = "PRG RAM\t\t: ",
        *restrict const chrromlbl = "CHR ROM\t\t: ",
        *restrict const chrramlbl = "CHR RAM\t\t: ";

    fprintf(f, "Mapper\t\t: %03u", self->ns_hdr.mapper_id);
    if (verbose) {
        fprintf(f, " (<Board Names>)\n");
        hr(f);
    } else {
        fprintf(f, "\n");
    }

    fprintf(f, "PRG ROM\t\t: %u%s\n", self->ns_hdr.prg_banks,
            verbose ? fullsize : "");
    if (self->ns_hdr.prg_ram) {
        fprintf(f, "%s%u%s\n", prgramlbl,
                self->ns_hdr.prg_rambanks == 0
                    ? 1u
                    : self->ns_hdr.prg_rambanks,
                verbose ? halfsize : "");
    } else if (verbose) {
        fprintf(f, "%sno\n", prgramlbl);
    }

    if (self->ns_hdr.chr_banks == 0) {
        if (verbose) {
            fprintf(f, "%sno\n", chrromlbl);
        }
        fprintf(f, "%s1%s\n", chrramlbl, verbose ? halfsize : "");
    } else {
        fprintf(f, "%s%u%s\n", chrromlbl, self->ns_hdr.chr_banks,
                verbose ? halfsize : "");
        if (verbose) {
            fprintf(f, "%sno\n", chrramlbl);
        }
    }

    if (verbose) {
        hr(f);
    }
    if (verbose || self->ns_hdr.trainer) {
        fprintf(f, "Trainer\t\t: %s\n", self->ns_hdr.trainer ? "yes" : "no");
    }
    if (verbose || self->ns_hdr.bus_conflicts) {
        fprintf(f, "Bus Conflicts\t: %s\n",
                self->ns_hdr.bus_conflicts ? "yes" : "no");
    }
}

//
// Public Interface
//

const char *cart_errstr(int error)
{
    switch (error) {
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

    struct cartridge *const self = malloc(sizeof *self);

    int error = detect_format(self, f);
    if (error == 0) {
        switch (self->format) {
        case CRTF_INES:
            error = parse_ines(self, f);
            break;
        default:
            error = parse_unknown(self, f);
            break;
        }
    }

    if (error == 0) {
        *c = self;
    } else {
        cart_free(self);
    }
    return error;
}

void cart_free(cart *self)
{
    free(self);
}

uint8_t *cart_prg_bank(cart *self)
{
    assert(self != NULL);

    return self->prg;
}

void cart_info_write(cart *self, FILE *f, bool verbose)
{
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

    snapshot->rom = cart_prg_bank(self);
}
