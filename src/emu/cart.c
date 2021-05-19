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
// NOTE: iNES file data is packed into 8 or 16 KB chunks
static const size_t Chunk = 0x2000,
                    DChunk = Chunk * 2;

enum cartformat {
#define X(s, n) CRTF_ENUM(s),
    CART_FORMAT_X
#undef X
};

// A NES game cartridge including PRG and CHR ROM banks,
// working/save-state RAM (PRG RAM), memory mapper circuitry,
// and metadata headers.

// TODO: ignoring following fields for now:
//  - hardcoded mirroring bits, need to know mapper to interpret these
//  - VS/Playchoice system indicator (does anyone care?)
//  - TV System (PAL ROMs don't seem to set the flags so again who cares)
//  - redundant indicators in byte 10
struct ines_header {
    uint8_t chr_chunks,     // CHR ROM chunk count (0 indicates CHR RAM)
            mapper_id,      // Mapper ID
            wram_chunks,    // WRAM chunk count (mapper may override this)
            prg_chunks;     // PRG double-chunk count
    bool bus_conflicts,     // Cart has bus conflicts
         wram,              // PRG RAM banks present
         trainer;           // Trainer data present
};

struct cartridge {
    enum cartformat format;         // Cart File Format
    union {
        struct ines_header ns_hdr;  // iNES Header
    };
    uint8_t *chr,                   // CHR RAM or ROM
            *prg,                   // PRG ROM
            *wram;                  // Working RAM
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
        self->format = CRTF_UNKNOWN;
    }

    // NOTE: reset back to beginning of file to fully parse detected format
    return fseek(f, 0, SEEK_SET) == 0 ? 0 : CART_IO_ERR;
}

static int load_chunks(uint8_t **mem, size_t size, FILE *f)
{
    *mem = calloc(size, sizeof **mem);
    fread(*mem, sizeof **mem, size, f);
    if (feof(f)) return CART_EOF;
    if (ferror(f)) return CART_IO_ERR;
    return 0;
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

    self->ns_hdr.prg_chunks = header[4];
    self->ns_hdr.chr_chunks = header[5];
    self->ns_hdr.wram = header[6] & 0x2;
    self->ns_hdr.trainer = header[6] & 0x4;
    self->ns_hdr.mapper_id = (header[6] >> 4) | (header[7] & 0xf0);
    self->ns_hdr.wram_chunks = header[8];
    self->ns_hdr.bus_conflicts = header[10] & 0x20;

    int err;
    if (self->ns_hdr.trainer) {
        // NOTE: skip 512 bytes of trainer data
        err = fseek(f, 512, SEEK_CUR);
        if (err != 0) return err;
    }

    if (self->ns_hdr.wram) {
        const size_t sz = (self->ns_hdr.wram_chunks == 0
                           ? 1
                           : self->ns_hdr.wram_chunks) * Chunk;
        self->wram = calloc(sz, sizeof *self->wram);
    } else {
        self->wram = NULL;
    }

    err = load_chunks(&self->prg, self->ns_hdr.prg_chunks * DChunk, f);
    if (err != 0) return err;

    if (self->ns_hdr.chr_chunks == 0) {
        self->chr = calloc(Chunk, sizeof *self->chr);
    } else {
        err = load_chunks(&self->chr, self->ns_hdr.chr_chunks * Chunk, f);
    }

    if (err == 0) {
        // TODO: we've found a ROM with extra bytes after CHR data
        assert(fgetc(f) == EOF && feof(f));
    }

    return err;
}

// TODO: load file contents into a single ROM bank and hope for the best
static int parse_unknown(struct cartridge *self, FILE *f)
{
    self->chr = self->wram = NULL;
    // TODO: assume a 32KB binary
    const size_t datasz = 2 * DChunk;
    self->prg = calloc(datasz, sizeof *self->prg);
    const size_t count = fread(self->prg, sizeof *self->prg, datasz, f);

    // TODO: feof should be unexpected EOF
    if (feof(f)) return 0;
    if (ferror(f)) return CART_IO_ERR;
    if (count == datasz) return CART_PRG_SIZE;
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

static void write_ines_info(struct cartridge *self, FILE *f, bool verbose)
{
    static const char
        *restrict const fullsize = " x 16KB",
        *restrict const halfsize = " x 8KB",
        *restrict const wramlbl = "WRAM\t\t: ",
        *restrict const chrromlbl = "CHR ROM\t\t: ",
        *restrict const chrramlbl = "CHR RAM\t\t: ";

    fprintf(f, "Mapper\t\t: %03u", self->ns_hdr.mapper_id);
    if (verbose) {
        fprintf(f, " (<Board Names>)\n");
        hr(f);
    } else {
        fprintf(f, "\n");
    }

    fprintf(f, "PRG ROM\t\t: %u%s\n", self->ns_hdr.prg_chunks,
            verbose ? fullsize : "");
    if (self->ns_hdr.wram) {
        fprintf(f, "%s%u%s\n", wramlbl,
                self->ns_hdr.wram_chunks == 0
                    ? 1u
                    : self->ns_hdr.wram_chunks,
                verbose ? halfsize : "");
    } else if (verbose) {
        fprintf(f, "%sno\n", wramlbl);
    }

    if (self->ns_hdr.chr_chunks == 0) {
        if (verbose) {
            fprintf(f, "%sno\n", chrromlbl);
        }
        fprintf(f, "%s1%s\n", chrramlbl, verbose ? halfsize : "");
    } else {
        fprintf(f, "%s%u%s\n", chrromlbl, self->ns_hdr.chr_chunks,
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
// Read/Write Interfaces
//

static bool simple_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    *d = ((uint8_t *)ctx)[addr & CpuRomAddrMask];
    return true;
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

    struct cartridge *const self = malloc(sizeof *self);

    int err = detect_format(self, f);
    if (err == 0) {
        switch (self->format) {
        case CRTF_INES:
            err = parse_ines(self, f);
            break;
        default:
            err = parse_unknown(self, f);
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
    free(self->chr);
    free(self->prg);
    free(self->wram);
    free(self);
}

uint8_t *cart_prg_bank(cart *self)
{
    assert(self != NULL);

    return self->prg;
}

int cart_connect_prg(cart *self, bus *b, uint16_t addr)
{
    assert(self != NULL);
    assert(b != NULL);

    // TODO: set up simple read-only for now
    return bus_set(b, addr,
                   (struct busdevice){.read = simple_read, .ctx = self->prg})
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

    snapshot->rom = self->prg;
}
