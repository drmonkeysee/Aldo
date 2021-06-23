//
//  dis.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#include "dis.h"

#include "emu/bytes.h"
#include "emu/decode.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *restrict const Mnemonics[] = {
#define X(s, ...) #s,
    DEC_INST_X
#undef X
};

static const int InstLens[] = {
#define X(s, b, ...) b,
    DEC_ADDRMODE_X
#undef X
};

#define STRTABLE(s) s##_StringTable

#define X(s, b, ...) static const char *restrict const STRTABLE(s)[] = { \
    __VA_ARGS__ \
};
    DEC_ADDRMODE_X
#undef X

static const char *restrict const *const StringTables[] = {
#define X(s, b, ...) STRTABLE(s),
    DEC_ADDRMODE_X
#undef X
};

static const char *interrupt_display(const struct console_state *snapshot)
{
    if (snapshot->datapath.exec_cycle == 6) return "CLR";
    if (snapshot->datapath.res == NIS_COMMITTED) return "(RES)";
    if (snapshot->datapath.nmi == NIS_COMMITTED) return "(NMI)";
    if (snapshot->datapath.irq == NIS_COMMITTED) return "(IRQ)";
    return "";
}

static int print_raw(uint16_t addr, const uint8_t *restrict bytes, int instlen,
                     char dis[restrict static DIS_INST_SIZE])
{
    int total, count;
    total = count = sprintf(dis, "$%04X: ", addr);
    if (count < 0) return DIS_ERR_FMT_FAIL;

    for (int i = 0; i < instlen; ++i) {
        count = sprintf(dis + total, "%02X ", bytes[i]);
        if (count < 0) return DIS_ERR_FMT_FAIL;
        total += count;
    }

    return total;
}

static int print_mnemonic(const struct decoded *dec,
                          const uint8_t *restrict bytes, int instlen,
                          char dis[restrict])
{
    int total, count;
    total = count = sprintf(dis, "%s ", Mnemonics[dec->instruction]);
    if (count < 0) return DIS_ERR_FMT_FAIL;

    const char *restrict const *const strtable = StringTables[dec->mode];
    switch (instlen) {
    case 1:
        // NOTE: nothing else to print, trim the trailing space
        dis[--count] = '\0';
        break;
    case 2:
        count = sprintf(dis + total, strtable[instlen - 1], bytes[1]);
        break;
    case 3:
        count = sprintf(dis + total, strtable[instlen - 1], batowr(bytes + 1));
        break;
    default:
        return DIS_ERR_INV_ADDRMD;
    }
    if (count < 0) return DIS_ERR_FMT_FAIL;
    return total + count;
}

enum duplicate_state {
    DUP_NONE,
    DUP_FIRST,
    DUP_TRUNCATE,
    DUP_SKIP,
    DUP_VERBOSE,
};

struct repeat_condition {
    uint32_t prev_bytes;
    enum duplicate_state state;
};

static void print_prg_line(const char *restrict dis, uint32_t curr_bytes,
                           size_t total, size_t banksize,
                           struct repeat_condition *repeat)
{
    if (repeat->state == DUP_VERBOSE) {
        puts(dis);
        return;
    }

    if (curr_bytes == repeat->prev_bytes) {
        switch (repeat->state) {
        case DUP_NONE:
            repeat->state = DUP_FIRST;
            puts(dis);
            break;
        case DUP_FIRST:
            repeat->state = DUP_TRUNCATE;
            // NOTE: if this is the last instruction in the PRG bank
            // skip printing the truncation indicator.
            if (total < banksize) {
                puts("*");
            }
            break;
        default:
            repeat->state = DUP_SKIP;
            break;
        }
    } else {
        puts(dis);
        repeat->prev_bytes = curr_bytes;
        repeat->state = DUP_NONE;
    }
}

static int print_prgbank(const struct bankview *bv, bool verbose)
{
    printf("Bank %zu (%zuKB)\n", bv->bank, bv->size >> BITWIDTH_1KB);
    puts("--------");

    int bytes_read = 0;
    struct repeat_condition repeat = {
        .state = verbose ? DUP_VERBOSE : DUP_NONE,
    };
    char dis[DIS_INST_SIZE];
    for (size_t total = 0; total < bv->size; total += bytes_read) {
        // TODO: how to pick correct start address?
        const uint16_t addr = MEMBLOCK_32KB + total;
        const uint8_t *const prgoffset = bv->mem + total;
        bytes_read = dis_inst(addr, prgoffset, bv->size - total, dis);
        if (bytes_read == 0) break;
        if (bytes_read < 0) {
            fprintf(stderr, "$%04X: Dis err (%d): %s\n", addr, bytes_read,
                    dis_errstr(bytes_read));
            return bytes_read;
        }
        // NOTE: convert current instruction bytes into easily comparable
        // value to check for repeats.
        uint32_t curr_bytes = 0;
        for (int i = 0; i < bytes_read; ++i) {
            curr_bytes |= prgoffset[i] << (8 * i);
        }
        print_prg_line(dis, curr_bytes, total, bv->size, &repeat);
    }

    // NOTE: always print the last line regardless of duplicate state
    // (if it hasn't already been printed).
    if (repeat.state == DUP_TRUNCATE || repeat.state == DUP_SKIP) {
        puts(dis);
    }

    return 0;
}

// NOTE: CHR bit-planes are 8 bits wide and 8 bytes tall; a CHR tile is an 8x8
// byte array of 2-bit palette-indexed pixels composed of two bit-planes where
// the first plane specifies the pixel low bit and the second plane specifies
// the pixel high bit.
// TODO: do these need to be literals
enum {
    CHR_PLANE_SIZE = 8,
    CHR_TILE_SPAN = 2 * CHR_PLANE_SIZE,
    CHR_TILE_SIZE = CHR_PLANE_SIZE * CHR_PLANE_SIZE,
};

// NOTE: BMP details from:
// https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-storage
enum {
    // NOTE: color depth and palette sizes; NES tiles are actually 2 bpp but
    // the closest the standard BMP format gets is 4; BMP palette size is also
    // 4, as are the number of colors representable at 2 bpp so conveniently
    // all color constants collapse into one value.
    BMP_COLOR_SIZE = 4,
    BMP_PALETTE_SIZE = BMP_COLOR_SIZE * BMP_COLOR_SIZE,

    // NOTE: bmp header sizes
    BMP_FILEHEADER_SIZE = 14,
    BMP_INFOHEADER_SIZE = 40,
    BMP_HEADER_SIZE = BMP_FILEHEADER_SIZE + BMP_INFOHEADER_SIZE
                      + BMP_PALETTE_SIZE,
};

static int measure_tile_sheet(size_t banksize, int32_t *restrict dim,
                              int32_t *restrict sections)
{
    // NOTE: display CHR tiles as 8x8 or 16x16 tile sections
    // labeled "Left" and "Right" containing the lower- and upper-half of
    // tiles respectively;
    // e.g. for an 8KB bank, Left: tiles 0-255, Right: tiles 256-511
    // in PPU address space this would correspond to
    // pattern tables $0000-$0FFF, $1000-$1FFF.
    *sections = 1;
    switch (banksize) {
    case MEMBLOCK_2KB:
        *sections = 2;
        // fallthrough
    case MEMBLOCK_1KB:
        *dim = 8;
        break;
    case MEMBLOCK_8KB:
        *sections = 2;
        // fallthrough
    case MEMBLOCK_4KB:
        *dim = 16;
        break;
    default:
        assert(((void)"INVALID CHR BANK SIZE", false));
        return DIS_ERR_CHRSZ;
    }
    return 0;
}

static void decode_tiles(const struct bankview *bv, size_t tilecount,
                         uint8_t *restrict tiles)
{
    for (size_t tileidx = 0; tileidx < tilecount; ++tileidx) {
        uint8_t *const tile = tiles + (tileidx * CHR_TILE_SIZE);
        const uint8_t *const planes = bv->mem + (tileidx * CHR_TILE_SPAN);
        for (size_t row = 0; row < CHR_PLANE_SIZE; ++row) {
            const uint8_t plane0 = planes[row],
                          plane1 = planes[row + CHR_PLANE_SIZE];
            for (size_t bit = 0; bit < CHR_PLANE_SIZE; ++bit) {
                // NOTE: tile origins are top-left so in order to pack pixels
                // left-to-right we need to decode each bit-plane row
                // least-significant bit last.
                const ptrdiff_t pixel = (CHR_PLANE_SIZE - bit - 1)
                                        + (row * CHR_PLANE_SIZE);
                assert(pixel >= 0);
                tile[pixel] = byte_getbit(plane0, bit)
                              | (byte_getbit(plane1, bit) << 1);
            }
        }
        // NOTE: did we process the entire CHR ROM?
        if (tileidx == tilecount - 1) {
            assert(planes + CHR_TILE_SPAN == bv->mem + bv->size);
        }
    }
}

// TODO: handle scale
static int write_tile_sheet(int32_t tilesdim, int32_t tile_sections,
                            const uint8_t *restrict tiles,
                            const char *restrict filename)
{
    const int32_t
        pxlsectiondim = tilesdim * CHR_PLANE_SIZE,
        pixelsx = pxlsectiondim * tile_sections,
        // NOTE: 4 bpp equals 8 pixels per 4 bytes; bmp pixel rows must then
        // be padded to the nearest 4-byte boundary.
        packedrow_size = ceil(pixelsx / 8.0) * 4;

    errno = 0;
    FILE *const bmpfile = fopen(filename, "wb");
    if (!bmpfile) return DIS_ERR_IO;

    // NOTE: write BMP header fields; BMP format is little-endian so use
    // byte arrays rather than structs to avoid arch-specific endianness.
    /*
     File Header
     struct BITMAPFILEHEADER {
        WORD  bfType;
        DWORD bfSize;
        WORD  bfReserved1;
        WORD  bfReserved2;
        DWORD bfOffBits;
     };
     */
    uint8_t fileheader[BMP_FILEHEADER_SIZE] = {'B', 'M'};       // bfType
    dwtoba(BMP_HEADER_SIZE + (pxlsectiondim * packedrow_size),  // bfSize
           fileheader + 2);
    dwtoba(BMP_HEADER_SIZE, fileheader + 10);                   // bfOffBits
    fwrite(fileheader, sizeof fileheader[0],
           sizeof fileheader / sizeof fileheader[0], bmpfile);

    /*
     Info Header
     struct BITMAPINFOHEADER {
        DWORD biSize;
        LONG  biWidth;
        LONG  biHeight;
        WORD  biPlanes;
        WORD  biBitCount;
        DWORD biCompression;
        DWORD biSizeImage;
        LONG  biXPelsPerMeter;
        LONG  biYPelsPerMeter;
        DWORD biClrUsed;
        DWORD biClrImportant;
     };
     */
    uint8_t infoheader[BMP_INFOHEADER_SIZE] = {
        BMP_INFOHEADER_SIZE,                // biSize
        [12] = 1,                           // biPlanes
        [14] = BMP_COLOR_SIZE,              // biBitCount
        [32] = BMP_COLOR_SIZE,              // biClrUsed
    };
    dwtoba(pixelsx, infoheader + 4);        // biWidth
    dwtoba(pxlsectiondim, infoheader + 8);  // biHeight
    fwrite(infoheader, sizeof infoheader[0],
           sizeof infoheader / sizeof infoheader[0], bmpfile);

    /*
     Palettes
     struct RGBQUAD {
        BYTE rgbBlue;
        BYTE rgbGreen;
        BYTE rgbRed;
        BYTE rgbReserved;
     }[BMP_COLOR_SIZE];
     */
    // TODO: replace with grayscale
    const uint8_t palettes[BMP_PALETTE_SIZE] = {
        0, 0, 0, 0,         // DK BLACK
        180, 16, 0, 0,      // DK BLUE
        180, 209, 235, 0,   // DK BEIGE
        37, 74, 229, 0,     // DK RED
    };
    fwrite(palettes, sizeof palettes[0], sizeof palettes / sizeof palettes[0],
           bmpfile);

    // NOTE: BMP pixels are written bottom-row first
    uint8_t *const packedrow = calloc(packedrow_size, sizeof *packedrow);
    for (int32_t tiley = tilesdim - 1; tiley >= 0; --tiley) {
        for (int32_t pixely = CHR_PLANE_SIZE - 1; pixely >= 0; --pixely) {
            for (int32_t tile_section = 0;
                 tile_section < tile_sections;
                 ++tile_section) {
                for (int32_t tilex = 0; tilex < tilesdim; ++tilex) {
                    // NOTE: tile_start is index of the first pixel
                    // in the current tile.
                    const size_t
                        tile_start = (tilex
                                      + (tiley * tilesdim)
                                      + (tile_section * tilesdim * tilesdim))
                                     * CHR_TILE_SIZE;
                    for (size_t pixelx = 0;
                         pixelx < CHR_PLANE_SIZE;
                         ++pixelx) {
                        // NOTE: pixeloffset is the pixel index in tile space,
                        // packedpixel is the pixel in bmp-row space.
                        const size_t
                            pixeloffset = pixelx + (pixely * CHR_PLANE_SIZE),
                            packedpixel = pixelx + (tilex * CHR_PLANE_SIZE)
                                          + (tile_section * pxlsectiondim);
                        const uint8_t pixel = tiles[tile_start + pixeloffset];
                        if (packedpixel % 2 == 0) {
                            packedrow[packedpixel / 2] = pixel << 4;
                        } else {
                            packedrow[packedpixel / 2] |= pixel;
                        }
                    }
                }
            }
            fwrite(packedrow, sizeof *packedrow,
                   packedrow_size / sizeof *packedrow, bmpfile);
        }
    }
    free(packedrow);

    fclose(bmpfile);
    return 0;
}

static int write_chrbank(const struct bankview *bv)
{
    int32_t tilesdim, tile_sections;
    int err = measure_tile_sheet(bv->size, &tilesdim, &tile_sections);
    if (err < 0) return err;

    char bmpfilename[16];
    if (sprintf(bmpfilename, "bank%03zu.bmp", bv->bank) < 0) return DIS_ERR_IO;

    printf("Bank %zu (%zuKB), %d x %d tiles (%d section%s): %s\n", bv->bank,
           bv->size >> BITWIDTH_1KB, tilesdim, tilesdim, tile_sections,
           tile_sections == 1 ? "" : "s", bmpfilename);

    const size_t tilecount = bv->size / CHR_TILE_SPAN;
    uint8_t *const tiles = calloc(tilecount * CHR_TILE_SIZE, sizeof *tiles);

    decode_tiles(bv, tilecount, tiles);
    err = write_tile_sheet(tilesdim, tile_sections, tiles, bmpfilename);

    free(tiles);
    return err;
}

//
// Public Interface
//

const char *dis_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case s: return e;
        DIS_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

int dis_inst(uint16_t addr, const uint8_t *restrict bytes, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE])
{
    assert(bytes != NULL);
    assert(dis != NULL);

    if (bytesleft <= 0) return 0;

    const struct decoded dec = Decode[*bytes];
    const int instlen = InstLens[dec.mode];
    if (bytesleft < instlen) return DIS_ERR_EOF;

    int count, total;
    total = count = print_raw(addr, bytes, instlen, dis);
    if (count < 0) return count;

    // NOTE: padding between raw bytes and disassembled instruction
    count = sprintf(dis + total, "%*s", (4 - instlen) * 3, "");
    if (count < 0) return DIS_ERR_FMT_FAIL;
    total += count;

    count = print_mnemonic(&dec, bytes, instlen, dis + total);
    if (count < 0) return count;
    total += count;

    assert((unsigned int)total < DIS_INST_SIZE);
    return instlen;
}

int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE])
{
    assert(snapshot != NULL);
    assert(dis != NULL);

    const struct decoded dec = Decode[snapshot->datapath.opcode];
    const int instlen = InstLens[dec.mode];
    if ((size_t)instlen > snapshot->mem.prglength) return DIS_ERR_EOF;

    int count, total;
    total = count = sprintf(dis, "%s ", Mnemonics[dec.instruction]);
    if (count < 0) return DIS_ERR_FMT_FAIL;

    const int max_offset = 1 + (instlen / 3),
              displayidx = snapshot->datapath.exec_cycle < max_offset
                           ? snapshot->datapath.exec_cycle
                           : max_offset;
    const char *const displaystr = StringTables[dec.mode][displayidx];
    switch (displayidx) {
    case 0:
        count = sprintf(dis + total, "%s", displaystr);
        break;
    case 1:
        count = dec.instruction == IN_BRK
                ? sprintf(dis + total, displaystr, interrupt_display(snapshot))
                : sprintf(dis + total, displaystr,
                          strlen(displaystr) > 0
                            ? snapshot->mem.prgview[1]
                            : 0);
        break;
    default:
        count = sprintf(dis + total, displaystr,
                        strlen(displaystr) > 0
                            ? batowr(snapshot->mem.prgview + 1)
                            : 0);
        break;
    }
    if (count < 0) return DIS_ERR_FMT_FAIL;
    total += count;

    assert((unsigned int)total < DIS_DATAP_SIZE);
    return total;
}

int dis_cart(cart *cart, const struct control *appstate)
{
    assert(cart != NULL);
    assert(appstate != NULL);

    puts(appstate->cartfile);
    cart_write_dis_header(cart, stdout);

    for (struct bankview bv = cart_prgbank(cart, 0);
         bv.mem;
         bv = cart_prgbank(cart, bv.bank + 1)) {
        puts("");
        const int err = print_prgbank(&bv, appstate->verbose);
        if (err < 0) return err;
    }
    return 0;
}

int dis_cart_chr(cart *cart)
{
    assert(cart != NULL);

    struct bankview bv = cart_chrbank(cart, 0);
    if (!bv.mem) return DIS_ERR_CHRROM;

    do {
        const int err = write_chrbank(&bv);
        if (err < 0) return err;
        bv = cart_chrbank(cart, bv.bank + 1);
    } while (bv.mem);

    return 0;
}
