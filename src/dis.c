//
//  dis.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#include "dis.h"

#include "bytes.h"
#include "decode.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *const restrict Mnemonics[] = {
#define X(s, d, f, ...) #s,
    DEC_INST_X
#undef X
};

static const char *const restrict Descriptions[] = {
#define X(s, d, f, ...) d,
    DEC_INST_X
#undef X
};

static const uint8_t Flags[] = {
#define X(s, d, f, ...) f,
    DEC_INST_X
#undef X
};

static const int InstLens[] = {
#define X(s, b, n, p, ...) b,
    DEC_ADDRMODE_X
#undef X
};

static const char *const restrict ModeNames[] = {
#define X(s, b, n, p, ...) n,
    DEC_ADDRMODE_X
#undef X
};

#define STRTABLE(s) s##_StringTable

#define X(s, b, n, p, ...) static const char *const restrict STRTABLE(s)[] = { \
    __VA_ARGS__ \
};
    DEC_ADDRMODE_X
#undef X

static const char *const restrict *const StringTables[] = {
#define X(s, b, n, p, ...) STRTABLE(s),
    DEC_ADDRMODE_X
#undef X
};

static const char *mnemonic(enum inst in)
{
    static const size_t sz = sizeof Mnemonics / sizeof Mnemonics[0];
    return 0 <= in && in < sz ? Mnemonics[in] : Mnemonics[IN_UDF];
}

static const char *description(enum inst in)
{
    static const size_t sz = sizeof Descriptions / sizeof Descriptions[0];
    return 0 <= in && in < sz ? Descriptions[in] : Descriptions[IN_UDF];
}

static const char *modename(enum addrmode am)
{
    static const size_t sz = sizeof ModeNames / sizeof ModeNames[0];
    return 0 <= am && am < sz ? ModeNames[am] : ModeNames[AM_IMP];
}

static uint8_t flags(enum inst in)
{
    static const size_t sz = sizeof Flags / sizeof Flags[0];
    return 0 <= in && in < sz ? Flags[in] : Flags[IN_UDF];
}

static const char *interrupt_display(const struct console_state *snapshot)
{
    if (snapshot->datapath.exec_cycle == 6) return "CLR";
    if (snapshot->datapath.res == NIS_COMMITTED) return "(RES)";
    if (snapshot->datapath.nmi == NIS_COMMITTED) return "(NMI)";
    if (snapshot->datapath.irq == NIS_COMMITTED) return "(IRQ)";
    return "";
}

static uint16_t interrupt_vector(const struct console_state *snapshot)
{
    if (snapshot->datapath.nmi == NIS_COMMITTED)
        return batowr(snapshot->mem.vectors);
    if (snapshot->datapath.res == NIS_COMMITTED)
        return batowr(snapshot->mem.vectors + 2);
    return batowr(snapshot->mem.vectors + 4);
}

static int print_raw(uint16_t addr, const struct dis_instruction *inst,
                     char dis[restrict static DIS_INST_SIZE])
{
    int total, count;
    total = count = sprintf(dis, "%04X: ", addr);
    if (count < 0) return DIS_ERR_FMT;

    for (size_t i = 0; i < inst->bv.size; ++i) {
        count = sprintf(dis + total, "%02X ", inst->bv.mem[i]);
        if (count < 0) return DIS_ERR_FMT;
        total += count;
    }

    return total;
}

static int print_operand(const struct dis_instruction *inst,
                         char dis[restrict])
{
    const char *const restrict *const strtable = StringTables[inst->d.mode];
    int count;
    switch (inst->bv.size) {
    case 1:
        dis[0] = '\0';
        count = 0;
        break;
    case 2:
        count = sprintf(dis, strtable[inst->bv.size - 1], inst->bv.mem[1]);
        break;
    case 3:
        count = sprintf(dis, strtable[inst->bv.size - 1],
                        batowr(inst->bv.mem + 1));
        break;
    default:
        assert(((void)"INVALID ADDR MODE LENGTH", false));
        return DIS_ERR_INV_ADDRMD;
    }
    if (count < 0) return DIS_ERR_FMT;
    return count;
}

static int print_instruction(const struct dis_instruction *inst,
                             char dis[restrict])
{
    int operation = sprintf(dis, "%s ", mnemonic(inst->d.instruction));
    if (operation < 0) return DIS_ERR_FMT;

    const int operand = print_operand(inst, dis + operation);
    if (operand < 0) return DIS_ERR_FMT;
    if (operand == 0) {
        // NOTE: no operand to print, trim the trailing space
        dis[--operation] = '\0';
    }
    return operation + operand;
}

struct repeat_condition {
    struct dis_instruction prev_inst;
    bool skip;
};

static void print_prg_line(const char *restrict dis, bool verbose,
                           const struct dis_instruction *curr_inst,
                           struct repeat_condition *repeat, FILE *f)
{
    if (verbose) {
        fprintf(f, "%s\n", dis);
        return;
    }

    if (dis_inst_equal(curr_inst, &repeat->prev_inst)) {
        // NOTE: only print placeholder on first duplicate seen
        if (!repeat->skip) {
            repeat->skip = true;
            fputs("*\n", f);
        }
    } else {
        fprintf(f, "%s\n", dis);
        repeat->prev_inst = *curr_inst;
        repeat->skip = false;
    }
}

static int print_prgbank(const struct bankview *bv, bool verbose, FILE *f)
{
    fprintf(f, "Bank %zu (%zuKB)\n", bv->bank, bv->size >> BITWIDTH_1KB);
    fputs("--------\n", f);

    struct repeat_condition repeat = {0};
    struct dis_instruction inst;
    char dis[DIS_INST_SIZE];
    // NOTE: by convention, count backwards from CPU vector locations
    uint16_t addr = MEMBLOCK_64KB - bv->size;

    int result;
    for (result = dis_parse_inst(bv, 0, &inst);
         result > 0;
         result = dis_parse_inst(bv, inst.bankoffset + inst.bv.size, &inst)) {
        result = dis_inst(addr, &inst, dis);
        if (result <= 0) break;
        print_prg_line(dis, verbose, &inst, &repeat, f);
        addr += inst.bv.size;
    }
    if (result < 0) return result;

    // NOTE: always print the last line even if it would normally be skipped
    if (repeat.skip) {
        fprintf(f, "%s\n", dis);
    }
    return 0;
}

// NOTE: CHR bit-planes are 8 bits wide and 8 bytes tall; a CHR tile is an 8x8
// byte array of 2-bit palette-indexed pixels composed of two bit-planes where
// the first plane specifies the pixel low bit and the second plane specifies
// the pixel high bit.
const size_t
    ChrPlaneSize = 8,
    ChrTileSpan = 2 * ChrPlaneSize,
    ChrTileSize = ChrPlaneSize * ChrPlaneSize;

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
        // Fallthrough
    case MEMBLOCK_1KB:
        *dim = 8;
        break;
    case MEMBLOCK_8KB:
        *sections = 2;
        // Fallthrough
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
        uint8_t *const tile = tiles + (tileidx * ChrTileSize);
        const uint8_t *const planes = bv->mem + (tileidx * ChrTileSpan);
        for (size_t row = 0; row < ChrPlaneSize; ++row) {
            const uint8_t
                plane0 = planes[row],
                plane1 = planes[row + ChrPlaneSize];
            for (size_t bit = 0; bit < ChrPlaneSize; ++bit) {
                // NOTE: tile origins are top-left so in order to pack pixels
                // left-to-right we need to decode each bit-plane row
                // least-significant bit last.
                const ptrdiff_t pixel = (ChrPlaneSize - bit - 1)
                                            + (row * ChrPlaneSize);
                assert(pixel >= 0);
                tile[pixel] = byte_getbit(plane0, bit)
                                | (byte_getbit(plane1, bit) << 1);
            }
        }
        // NOTE: did we process the entire CHR ROM?
        if (tileidx == tilecount - 1) {
            assert(planes + ChrTileSpan == bv->mem + bv->size);
        }
    }
}

static void fill_tile_sheet_row(uint8_t *restrict packedrow,
                                int32_t tiley, int32_t pixely,
                                int32_t tilesdim, int32_t tile_sections,
                                int scale, int32_t section_pxldim,
                                const uint8_t *restrict tiles)
{
    for (int32_t section = 0; section < tile_sections; ++section) {
        for (int32_t tilex = 0; tilex < tilesdim; ++tilex) {
            // NOTE: tile_start is index of the first pixel
            // in the current tile.
            const size_t tile_start = (tilex + (tiley * tilesdim)
                                       + (section * tilesdim * tilesdim))
                                        * ChrTileSize;
            for (size_t pixelx = 0; pixelx < ChrPlaneSize; ++pixelx) {
                // NOTE: pixeloffset is the pixel index in tile space,
                // packedpixel is the pixel in (prescaled) bmp-row space.
                const size_t
                    pixeloffset = pixelx + (pixely * ChrPlaneSize),
                    packedpixel = pixelx + (tilex * ChrPlaneSize)
                                    + (section * section_pxldim);
                const uint8_t pixel = tiles[tile_start + pixeloffset];
                for (int scalex = 0; scalex < scale; ++scalex) {
                    const size_t scaledpixel = scalex + (packedpixel * scale);
                    if (scaledpixel % 2 == 0) {
                        packedrow[scaledpixel / 2] = pixel << 4;
                    } else {
                        packedrow[scaledpixel / 2] |= pixel;
                    }
                }
            }
        }
    }
}

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

static int write_tile_sheet(int32_t tilesdim, int32_t tile_sections, int scale,
                            const uint8_t *restrict tiles, FILE *bmpfile)
{
    const int32_t
        section_pxldim = tilesdim * ChrPlaneSize,
        bmph = section_pxldim * scale,
        bmpw = section_pxldim * tile_sections * scale,
        // NOTE: 4 bpp equals 8 pixels per 4 bytes; bmp pixel rows must then
        // be padded to the nearest 4-byte boundary.
        packedrow_size = ceil(bmpw / 8.0) * 4;

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
    uint8_t fileheader[BMP_FILEHEADER_SIZE] = {'B', 'M'};   // bfType
    dwtoba(BMP_HEADER_SIZE + (bmph * packedrow_size),       // bfSize
           fileheader + 2);
    dwtoba(BMP_HEADER_SIZE, fileheader + 10);               // bfOffBits
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
        BMP_INFOHEADER_SIZE,        // biSize
        [12] = 1,                   // biPlanes
        [14] = BMP_COLOR_SIZE,      // biBitCount
        [32] = BMP_COLOR_SIZE,      // biClrUsed
    };
    dwtoba(bmpw, infoheader + 4);   // biWidth
    dwtoba(bmph, infoheader + 8);   // biHeight
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
    // NOTE: no fixed palette for tiles so use grayscale;
    // fun fact, this is the original Gameboy palette.
    const uint8_t palettes[BMP_PALETTE_SIZE] = {
        0, 0, 0, 0,         // #000000
        103, 103, 103, 0,   // #676767
        182, 182, 182, 0,   // #b6b6b6
        255, 255, 255, 0,   // #ffffff
    };
    fwrite(palettes, sizeof palettes[0], sizeof palettes / sizeof palettes[0],
           bmpfile);

    // NOTE: BMP pixels are written bottom-row first
    uint8_t *const packedrow = calloc(packedrow_size, sizeof *packedrow);
    for (int32_t tiley = tilesdim - 1; tiley >= 0; --tiley) {
        for (int32_t pixely = ChrPlaneSize - 1; pixely >= 0; --pixely) {
            for (int scaley = 0; scaley < scale; ++scaley) {
                fill_tile_sheet_row(packedrow, tiley, pixely, tilesdim,
                                    tile_sections, scale, section_pxldim,
                                    tiles);
                fwrite(packedrow, sizeof *packedrow,
                       packedrow_size / sizeof *packedrow, bmpfile);
            }
        }
    }
    free(packedrow);

    return 0;
}

static int write_chrtiles(const struct bankview *bv, int32_t tilesdim,
                          int32_t tile_sections, int scale, FILE *bmpfile)
{
    const size_t tilecount = bv->size / ChrTileSpan;
    uint8_t *const tiles = calloc(tilecount * ChrTileSize, sizeof *tiles);
    decode_tiles(bv, tilecount, tiles);
    const int err = write_tile_sheet(tilesdim, tile_sections, scale, tiles,
                                     bmpfile);
    free(tiles);
    return err;
}

static int write_chrbank(const struct bankview *bv, int scale,
                         const char *restrict prefix, FILE *output)
{
    int32_t tilesdim, tile_sections;
    int err = measure_tile_sheet(bv->size, &tilesdim, &tile_sections);
    if (err < 0) return err;

    char bmpfilename[128];
    prefix = prefix && strlen(prefix) > 0 ? prefix : "bank";
    if (snprintf(bmpfilename, sizeof bmpfilename, "%.120s%03zu.bmp", prefix,
                 bv->bank) < 0) return DIS_ERR_FMT;
    FILE *const bmpfile = fopen(bmpfilename, "wb");
    if (!bmpfile) return DIS_ERR_ERNO;

    err = write_chrtiles(bv, tilesdim, tile_sections, scale, bmpfile);
    fclose(bmpfile);

    if (err == 0) {
        fprintf(output, "Bank %zu (%zuKB), %d x %d tiles (%d section%s)",
                bv->bank, bv->size >> BITWIDTH_1KB, tilesdim, tilesdim,
                tile_sections, tile_sections == 1 ? "" : "s");
        if (scale > 1) {
            fprintf(output, " (%dx scale)", scale);
        }
        fprintf(output, ": %s\n", bmpfilename);
    }

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

int dis_parse_inst(const struct bankview *bv, size_t at,
                   struct dis_instruction *parsed)
{
    assert(bv != NULL);
    assert(parsed != NULL);

    *parsed = (struct dis_instruction){0};
    if (!bv->mem) return DIS_ERR_PRGROM;
    if (at >= bv->size) return 0;

    const uint8_t opcode = bv->mem[at];
    const struct decoded dec = Decode[opcode];
    const int instlen = InstLens[dec.mode];
    if ((size_t)instlen > bv->size - at) return DIS_ERR_EOF;

    *parsed = (struct dis_instruction){
        at,
        {bv->bank, instlen, bv->mem + at},
        dec,
    };
    return instlen;
}

int dis_parsemem_inst(size_t size, const uint8_t mem[restrict size],
                      size_t at, struct dis_instruction *parsed)
{
    const struct bankview bv = {
        .mem = mem,
        .size = size,
    };
    return dis_parse_inst(&bv, at, parsed);
}

const char *dis_inst_mnemonic(const struct dis_instruction *inst)
{
    assert(inst != NULL);

    return mnemonic(inst->d.instruction);
}

const char *dis_inst_description(const struct dis_instruction *inst)
{
    assert(inst != NULL);

    return description(inst->d.instruction);
}

const char *dis_inst_addrmode(const struct dis_instruction *inst)
{
    assert(inst != NULL);

    return modename(inst->d.mode);
}

uint8_t dis_inst_flags(const struct dis_instruction *inst)
{
    assert(inst != NULL);

    return flags(inst->d.instruction);
}

int dis_inst_operand(const struct dis_instruction *inst,
                     char dis[restrict static DIS_OPERAND_SIZE])
{
    assert(inst != NULL);
    assert(dis != NULL);

    if (!inst->bv.mem) {
        dis[0] = '\0';
        return 0;
    }

    const int count = print_operand(inst, dis);

    assert((unsigned int)count < DIS_OPERAND_SIZE);
    return count;
}

bool dis_inst_equal(const struct dis_instruction *lhs,
                    const struct dis_instruction *rhs)
{
    return lhs && rhs && lhs->bv.size == rhs->bv.size
            && memcmp(lhs->bv.mem, rhs->bv.mem, lhs->bv.size) == 0;
}

int dis_inst(uint16_t addr, const struct dis_instruction *inst,
             char dis[restrict static DIS_INST_SIZE])
{
    assert(inst != NULL);
    assert(dis != NULL);

    if (!inst->bv.mem) return 0;

    int count, total;
    total = count = print_raw(addr, inst, dis);
    if (count < 0) return count;

    // NOTE: padding between raw bytes and disassembled instruction
    count = sprintf(dis + total, "%*s", ((3 - (int)inst->bv.size) * 3) + 1,
                    "");
    if (count < 0) return DIS_ERR_FMT;
    total += count;

    count = print_instruction(inst, dis + total);
    if (count < 0) return count;
    if (inst->d.unofficial && total > 0) {
        dis[total - 1] = '*';
    }
    total += count;

    assert((unsigned int)total < DIS_INST_SIZE);
    return total;
}

int dis_peek(uint16_t addr, struct mos6502 *cpu,
             const struct console_state *snapshot,
             char dis[restrict static DIS_PEEK_SIZE])
{
    assert(cpu != NULL);
    assert(snapshot != NULL);
    assert(dis != NULL);

    int total = 0;
    const char *const interrupt = interrupt_display(snapshot);
    if (strlen(interrupt) > 0) {
        int count = total = sprintf(dis, "%s > ", interrupt);
        if (count < 0) return DIS_ERR_FMT;
        const char *fmt;
        uint16_t vector;
        if (snapshot->datapath.res == NIS_COMMITTED
            && snapshot->debugger.resvector_override >= 0) {
            fmt = "!%04X";
            vector = snapshot->debugger.resvector_override;
        } else {
            fmt = "%04X";
            vector = interrupt_vector(snapshot);
        }
        count = sprintf(dis + total, fmt, vector);
        if (count < 0) return DIS_ERR_FMT;
        total += count;
    } else {
        cpu_ctx *const peekctx = cpu_peek_start(cpu);
        const struct cpu_peekresult peek = cpu_peek(cpu, addr);
        cpu_peek_end(cpu, peekctx);
        switch (peek.mode) {
#define XPEEK(...) sprintf(dis, __VA_ARGS__)
#define X(s, b, n, p, ...) case AM_ENUM(s): total = p; break;
            DEC_ADDRMODE_X
#undef X
#undef XPEEK
        default:
            assert(((void)"BAD ADDRMODE PEEK", false));
            return DIS_ERR_INV_ADDRMD;
        }
        if (total < 0) return DIS_ERR_FMT;
        if (peek.busfault) {
            char *const data = strrchr(dis, '=');
            if (data) {
                // NOTE: verify last '=' leaves enough space for "FLT"
                assert(dis + DIS_PEEK_SIZE - data >= 6);
                strcpy(data + 2, "FLT");
                ++total;
            }
        }
    }

    assert((unsigned int)total < DIS_PEEK_SIZE);
    return total;
}

int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE])
{
    assert(snapshot != NULL);
    assert(dis != NULL);

    struct dis_instruction inst;
    const int err = dis_parsemem_inst(snapshot->mem.prglength,
                                      snapshot->mem.currprg, 0, &inst);
    if (err < 0) return err;

    int count, total;
    total = count = sprintf(dis, "%s ", mnemonic(inst.d.instruction));
    if (count < 0) return DIS_ERR_FMT;

    const size_t
        max_offset = 1 + (inst.bv.size / 3),
        displayidx = snapshot->datapath.exec_cycle < max_offset
                        ? snapshot->datapath.exec_cycle
                        : max_offset;
    const char *const displaystr = StringTables[inst.d.mode][displayidx];
    switch (displayidx) {
    case 0:
        count = sprintf(dis + total, "%s", displaystr);
        break;
    case 1:
        count = inst.d.instruction == IN_BRK
                    ? sprintf(dis + total, displaystr,
                              interrupt_display(snapshot))
                    : sprintf(dis + total, displaystr,
                              strlen(displaystr) > 0 ? inst.bv.mem[1] : 0);
        break;
    default:
        count = sprintf(dis + total, displaystr,
                        strlen(displaystr) > 0 ? batowr(inst.bv.mem + 1) : 0);
        break;
    }
    if (count < 0) return DIS_ERR_FMT;
    total += count;

    assert((unsigned int)total < DIS_DATAP_SIZE);
    return total;
}

int dis_cart_prg(cart *cart, const struct control *appstate, FILE *f)
{
    assert(cart != NULL);
    assert(appstate != NULL);
    assert(f != NULL);

    struct bankview bv = cart_prgbank(cart, 0);
    if (!bv.mem) return DIS_ERR_PRGROM;

    fprintf(f, "%s\n", appstate->cartfile);
    cart_write_dis_header(cart, f);

    do {
        fputc('\n', f);
        const int err = print_prgbank(&bv, appstate->verbose, f);
        if (err < 0) {
            fprintf(stderr, "Dis err (%d): %s\n", err, dis_errstr(err));
        }
        bv = cart_prgbank(cart, bv.bank + 1);
    } while (bv.mem);

    return 0;
}

int dis_cart_chrbank(const struct bankview *bv, int scale, FILE *f)
{
    assert(bv != NULL);
    assert(f != NULL);

    if (!bv->mem) return DIS_ERR_CHRROM;
    if (scale <= 0) return DIS_ERR_CHRSCL;

    int32_t tilesdim, tile_sections;
    const int err = measure_tile_sheet(bv->size, &tilesdim, &tile_sections);
    if (err < 0) return err;

    return write_chrtiles(bv, tilesdim, tile_sections, scale, f);
}

int dis_cart_chr(cart *cart, const struct control *appstate, FILE *output)
{
    assert(cart != NULL);
    assert(appstate != NULL);
    assert(output != NULL);

    struct bankview bv = cart_chrbank(cart, 0);
    if (!bv.mem) return DIS_ERR_CHRROM;

    do {
        const int err = write_chrbank(&bv, appstate->chrscale,
                                      appstate->chrdecode_prefix, output);
        if (err < 0) {
            fprintf(stderr, "Chr err (%d): %s\n", err, dis_errstr(err));
        }
        bv = cart_chrbank(cart, bv.bank + 1);
    } while (bv.mem);

    return 0;
}
