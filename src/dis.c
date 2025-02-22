//
//  dis.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#include "dis.h"

#include "bytes.h"
#include "cpu.h"
#include "ctrlsignal.h"
#include "haltexpr.h"
#include "ppu.h"
#include "snapshot.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *const restrict Mnemonics[] = {
#define X(s, d, f, ...) #s,
    ALDO_DEC_INST_X
#undef X
};

static const char *const restrict Descriptions[] = {
#define X(s, d, f, ...) d,
    ALDO_DEC_INST_X
#undef X
};

static const uint8_t Flags[] = {
#define X(s, d, f, ...) f,
    ALDO_DEC_INST_X
#undef X
};

static const int InstLens[] = {
#define X(s, b, n, p, ...) b,
    ALDO_DEC_ADDRMODE_X
#undef X
};

static const char *const restrict ModeNames[] = {
#define X(s, b, n, p, ...) n,
    ALDO_DEC_ADDRMODE_X
#undef X
};

#define STRTABLE(s) s##_StringTable

#define X(s, b, n, p, ...) static const char *const restrict STRTABLE(s)[] = { \
    __VA_ARGS__ \
};
    ALDO_DEC_ADDRMODE_X
#undef X

static const char *const restrict *const StringTables[] = {
#define X(s, b, n, p, ...) STRTABLE(s),
    ALDO_DEC_ADDRMODE_X
#undef X
};

static const char *mnemonic(enum aldo_inst in)
{
    return 0 <= in && in < aldo_arrsz(Mnemonics)
            ? Mnemonics[in]
            : Mnemonics[ALDO_IN_UDF];
}

static const char *description(enum aldo_inst in)
{
    return 0 <= in && in < aldo_arrsz(Descriptions)
            ? Descriptions[in]
            : Descriptions[ALDO_IN_UDF];
}

static const char *modename(enum aldo_addrmode am)
{
    return 0 <= am && am < aldo_arrsz(ModeNames)
            ? ModeNames[am]
            : ModeNames[ALDO_AM_IMP];
}

static uint8_t flags(enum aldo_inst in)
{
    return 0 <= in && in < aldo_arrsz(Flags) ? Flags[in] : Flags[ALDO_IN_UDF];
}

static const char *interrupt_display(const struct aldo_snapshot *snp)
{
    if (snp->cpu.datapath.opcode != Aldo_BrkOpcode) return "";
    if (snp->cpu.datapath.exec_cycle == 6) return "CLR";
    if (snp->cpu.datapath.rst == ALDO_SIG_COMMITTED) return "(RST)";
    if (snp->cpu.datapath.nmi == ALDO_SIG_COMMITTED) return "(NMI)";
    if (snp->cpu.datapath.irq == ALDO_SIG_COMMITTED) return "(IRQ)";
    return "";
}

static uint16_t interrupt_vector(const struct aldo_snapshot *snp)
{
    if (snp->cpu.datapath.nmi == ALDO_SIG_COMMITTED)
        return aldo_batowr(snp->prg.vectors);
    if (snp->cpu.datapath.rst == ALDO_SIG_COMMITTED)
        return aldo_batowr(snp->prg.vectors + 2);
    return aldo_batowr(snp->prg.vectors + 4);
}

static int print_raw(uint16_t addr, const struct aldo_dis_instruction *inst,
                     char dis[restrict static ALDO_DIS_INST_SIZE])
{
    int total, count;
    total = count = sprintf(dis, "%04X: ", addr);
    if (count < 0) return ALDO_DIS_ERR_FMT;

    for (size_t i = 0; i < inst->bv.size; ++i) {
        count = sprintf(dis + total, "%02X ", inst->bv.mem[i]);
        if (count < 0) return ALDO_DIS_ERR_FMT;
        total += count;
    }

    return total;
}

static int print_operand(const struct aldo_dis_instruction *inst,
                         char dis[restrict])
{
    const char *const restrict *strtable = StringTables[inst->d.mode];
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
                        aldo_batowr(inst->bv.mem + 1));
        break;
    default:
        assert(((void)"INVALID ADDR MODE LENGTH", false));
        return ALDO_DIS_ERR_INV_ADDRMD;
    }
    if (count < 0) return ALDO_DIS_ERR_FMT;
    return count;
}

static int print_instruction(const struct aldo_dis_instruction *inst,
                             char dis[restrict])
{
    int operation = sprintf(dis, "%s ", mnemonic(inst->d.instruction));
    if (operation < 0) return ALDO_DIS_ERR_FMT;

    int operand = print_operand(inst, dis + operation);
    if (operand < 0) return ALDO_DIS_ERR_FMT;
    if (operand == 0) {
        // NOTE: no operand to print, trim the trailing space
        dis[--operation] = '\0';
    }
    return operation + operand;
}

struct repeat_condition {
    struct aldo_dis_instruction prev_inst;
    bool skip;
};

static int print_prg_line(const char *restrict dis, bool verbose,
                          const struct aldo_dis_instruction *curr_inst,
                          struct repeat_condition *repeat, FILE *f)
{
    if (verbose && fprintf(f, "%s\n", dis) < 0) goto io_failure;

    if (aldo_dis_inst_equal(curr_inst, &repeat->prev_inst)) {
        // NOTE: only print placeholder on first duplicate seen
        if (!repeat->skip) {
            repeat->skip = true;
            if (fputs("*\n", f) == EOF) goto io_failure;
        }
    } else {
        if (fprintf(f, "%s\n", dis) < 0) goto io_failure;
        repeat->prev_inst = *curr_inst;
        repeat->skip = false;
    }
    return 0;

io_failure:
    return ALDO_DIS_ERR_IO;
}

static int print_prgblock(const struct aldo_blockview *bv, bool verbose,
                          FILE *f)
{
    int io_err = fprintf(f, "Block %zu (%zuKB)\n", bv->ord,
                         bv->size >> ALDO_BITWIDTH_1KB);
    if (io_err < 0) return ALDO_DIS_ERR_IO;
    if (fputs("--------\n", f) == EOF) return ALDO_DIS_ERR_IO;

    struct repeat_condition repeat = {0};
    struct aldo_dis_instruction inst;
    char dis[ALDO_DIS_INST_SIZE];
    // NOTE: by convention, count backwards from CPU vector locations
    assert(bv->size <= ALDO_MEMBLOCK_64KB);
    uint16_t addr = (uint16_t)(ALDO_MEMBLOCK_64KB - bv->size);

    int result;
    for (result = aldo_dis_parse_inst(bv, 0, &inst);
         result > 0;
         result = aldo_dis_parse_inst(bv, inst.offset + inst.bv.size, &inst)) {
        result = aldo_dis_inst(addr, &inst, dis);
        if (result <= 0) break;
        io_err = print_prg_line(dis, verbose, &inst, &repeat, f);
        if (io_err < 0) break;
        addr += (uint16_t)inst.bv.size;
    }
    if (result < 0) return result;
    if (io_err < 0) return io_err;

    // NOTE: always print the last line even if it would normally be skipped
    if (repeat.skip && fprintf(f, "%s\n", dis) < 0) return ALDO_DIS_ERR_IO;
    return 0;
}

// NOTE: hardcode max scale to ~7MB bmp file size
static const int ScaleGuard = 20;

static int measure_tile_sheet(size_t banksize, uint32_t *restrict dim,
                              uint32_t *restrict sections)
{
    /*
     * Display CHR tiles as 8x8 or 16x16 tile sections
     * labeled "Left" and "Right" containing the lower- and upper-half of
     * tiles respectively.
     *
     * e.g. for an 8KB bank, Left: tiles 0-255, Right: tiles 256-511
     * in PPU address space this would correspond to
     * pattern tables $0000-$0FFF, $1000-$1FFF.
     */
    *sections = 1;
    switch (banksize) {
    case ALDO_MEMBLOCK_2KB:
        *sections = 2;
        // Fallthrough
    case ALDO_MEMBLOCK_1KB:
        *dim = 8;
        break;
    case ALDO_MEMBLOCK_8KB:
        *sections = 2;
        // Fallthrough
    case ALDO_MEMBLOCK_4KB:
        *dim = 16;
        break;
    default:
        assert(((void)"INVALID CHR BANK SIZE", false));
        return ALDO_DIS_ERR_CHRSZ;
    }
    return 0;
}

// NOTE: BMP details from:
// https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-storage
enum {
    // NOTE: color depth and palette sizes; NES tiles are actually 2 bpp but
    // the closest the standard BMP format gets is 4; BMP palette size is also
    // 4, as are the number of colors representable at 2 bpp so conveniently
    // all color constants collapse into one value.
    BMP_BITS_PER_PIXEL = 4,
    BMP_PALETTE_SIZE = BMP_BITS_PER_PIXEL * BMP_BITS_PER_PIXEL,

    // NOTE: bmp header sizes
    BMP_FILEHEADER_SIZE = 14,
    BMP_INFOHEADER_SIZE = 40,
    BMP_HEADER_SIZE = BMP_FILEHEADER_SIZE + BMP_INFOHEADER_SIZE
                        + BMP_PALETTE_SIZE,
};

static void fill_tile_sheet_row(uint8_t *restrict packedrow,
                                uint32_t tiley, uint32_t pixely,
                                uint32_t tilesdim, uint32_t tile_sections,
                                uint32_t scale, uint32_t section_pxldim,
                                const struct aldo_blockview *bv)
{
    for (uint32_t section = 0; section < tile_sections; ++section) {
        for (uint32_t tilex = 0; tilex < tilesdim; ++tilex) {
            // NOTE: get the index in tile space first, then calculate the
            // byte index in CHR space for the given tile's current row.
            size_t
                tileidx = tilex + (tiley * tilesdim)
                            + (section * tilesdim * tilesdim),
                chr_row = (tileidx * ALDO_CHR_TILE_STRIDE) + pixely;
            uint8_t
                plane0 = bv->mem[chr_row],
                plane1 = bv->mem[chr_row + ALDO_CHR_TILE_DIM];
            // NOTE: 8 2-bit pixels make a single CHR tile row; each 2-bit
            // pixel will be expanded to 4-bits and packed 2 pixels per byte
            // for a 4 bpp BMP.
            uint16_t pixelrow = aldo_byteshuffle(plane0, plane1);
            for (size_t pixelx = 0; pixelx < ALDO_CHR_TILE_DIM; ++pixelx) {
                // NOTE: packedpixel is the pixel in (prescaled) bmp-row space;
                // pixelidx is the 2-bit slice of pixelrow that maps to the
                // packedpixel; BMP layout goes from left-to-right so pixelidx
                // goes "backwards" from MSBs to LSBs.
                size_t
                    packedpixel = pixelx + (tilex * ALDO_CHR_TILE_DIM)
                                    + (section * section_pxldim),
                    pixelidx = ALDO_CHR_TILE_STRIDE - ((pixelx + 1) * 2);
                uint8_t pixel =
                    (uint8_t)((pixelrow & (0x3 << pixelidx)) >> pixelidx);
                assert(pixelidx < ALDO_CHR_TILE_STRIDE);
                for (uint32_t scalex = 0; scalex < scale; ++scalex) {
                    size_t scaledpixel = scalex + (packedpixel * scale);
                    if (scaledpixel % 2 == 0) {
                        packedrow[scaledpixel / 2] =
                            (uint8_t)(pixel << BMP_BITS_PER_PIXEL);
                    } else {
                        packedrow[scaledpixel / 2] |= pixel;
                    }
                }
            }
        }
    }
}

static int write_chrtiles(const struct aldo_blockview *bv, uint32_t tilesdim,
                          uint32_t tile_sections, uint32_t scale,
                          FILE *bmpfile)
{
    uint32_t
        section_pxldim = tilesdim * ALDO_CHR_TILE_DIM,
        bmph = section_pxldim * scale,
        bmpw = section_pxldim * tile_sections * scale,
        // NOTE: 4 bpp equals 8 pixels per 4 bytes; bmp pixel rows must then
        // be padded to the nearest 4-byte boundary.
        packedrow_size = (uint32_t)(ceil(bmpw / 8.0) * 4);

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
    aldo_dwtoba(BMP_HEADER_SIZE + (bmph * packedrow_size),  // bfSize
                fileheader + 2);
    aldo_dwtoba(BMP_HEADER_SIZE, fileheader + 10);          // bfOffBits
    size_t
        witems = aldo_arrsz(fileheader),
        wcount = fwrite(fileheader, sizeof fileheader[0], witems, bmpfile);
    if (wcount < witems) return ALDO_DIS_ERR_IO;

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
        [14] = BMP_BITS_PER_PIXEL,  // biBitCount
        [32] = BMP_BITS_PER_PIXEL,  // biClrUsed
    };
    aldo_dwtoba(bmpw, infoheader + 4);   // biWidth
    aldo_dwtoba(bmph, infoheader + 8);   // biHeight
    witems = aldo_arrsz(infoheader);
    wcount = fwrite(infoheader, sizeof infoheader[0], witems, bmpfile);
    if (wcount < witems) return ALDO_DIS_ERR_IO;

    /*
     Palettes
     struct RGBQUAD {
        BYTE rgbBlue;
        BYTE rgbGreen;
        BYTE rgbRed;
        BYTE rgbReserved;
     }[BMP_BITS_PER_PIXEL];
     */
    // NOTE: no fixed palette for tiles so use grayscale;
    // fun fact, this is the original Gameboy palette.
    static const uint8_t palettes[BMP_PALETTE_SIZE] = {
        0, 0, 0, 0,         // #000000
        103, 103, 103, 0,   // #676767
        182, 182, 182, 0,   // #b6b6b6
        255, 255, 255, 0,   // #ffffff
    };
    witems = aldo_arrsz(palettes);
    wcount = fwrite(palettes, sizeof palettes[0], witems, bmpfile);
    if (wcount < witems) return ALDO_DIS_ERR_IO;

    // NOTE: BMP pixels are written bottom-row first
    uint8_t *packedrow = calloc(packedrow_size, sizeof *packedrow);
    if (!packedrow) return ALDO_DIS_ERR_ERNO;

    for (int32_t tiley = (int32_t)(tilesdim - 1); tiley >= 0; --tiley) {
        for (int32_t pixely = (int32_t)(ALDO_CHR_TILE_DIM - 1);
             pixely >= 0;
             --pixely) {
            for (uint32_t scaley = 0; scaley < scale; ++scaley) {
                fill_tile_sheet_row(packedrow, (uint32_t)tiley,
                                    (uint32_t)pixely, tilesdim, tile_sections,
                                    scale, section_pxldim, bv);
                witems = packedrow_size / sizeof *packedrow;
                wcount = fwrite(packedrow, sizeof *packedrow, witems, bmpfile);
                if (wcount < witems) goto cleanup;
            }
        }
    }
cleanup:
    free(packedrow);
    return wcount < witems ? ALDO_DIS_ERR_ERNO : 0;
}

static int write_chrblock(const struct aldo_blockview *bv, uint32_t scale,
                          const char *restrict bmpfilename, FILE *output)
{
    uint32_t tilesdim, tile_sections;
    int err = measure_tile_sheet(bv->size, &tilesdim, &tile_sections);
    if (err < 0) return err;

    FILE *bmpfile = fopen(bmpfilename, "wb");
    if (!bmpfile) return ALDO_DIS_ERR_ERNO;

    err = write_chrtiles(bv, tilesdim, tile_sections, scale, bmpfile);
    fclose(bmpfile);

    if (err == 0) {
        err = fprintf(output,
                      "Block %zu (%zuKB), %u x %u tiles (%u section%s)",
                      bv->ord, bv->size >> ALDO_BITWIDTH_1KB, tilesdim,
                      tilesdim, tile_sections, tile_sections == 1 ? "" : "s");
        if (err < 0) return ALDO_DIS_ERR_IO;
        if (scale > 1 && fprintf(output, " (%ux scale)", scale) < 0)
            return ALDO_DIS_ERR_IO;
        if (fprintf(output, ": %s\n", bmpfilename) < 0) return ALDO_DIS_ERR_IO;
    }

    return err;
}

static struct aldo_peekresult run_peek(struct aldo_mos6502 *cpu,
                                       struct aldo_rp2c02 *ppu)
{
    struct aldo_rp2c02 ppu_restore = *ppu;
    struct aldo_mos6502 cpu_restore;
    struct aldo_peekresult peek = aldo_cpu_peek_start(cpu, &cpu_restore);
    do {
        for (int i = 0; i < Aldo_PpuRatio; ++i) {
            aldo_ppu_cycle(ppu);
        }
        aldo_cpu_peek(cpu, &peek);
    } while (!peek.done);
    aldo_cpu_peek_end(cpu, &cpu_restore);
    *ppu = ppu_restore;
    return peek;
}

//
// MARK: - Public Interface
//

const int Aldo_MinChrScale = 1, Aldo_MaxChrScale = 10;

const char *aldo_dis_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case ALDO_##s: return e;
        ALDO_DIS_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

int aldo_dis_parse_inst(const struct aldo_blockview *bv, size_t at,
                        struct aldo_dis_instruction *parsed)
{
    assert(bv != NULL);
    assert(parsed != NULL);

    *parsed = (struct aldo_dis_instruction){0};
    if (!bv->mem) return ALDO_DIS_ERR_PRGROM;
    if (at >= bv->size) return 0;

    uint8_t opcode = bv->mem[at];
    struct aldo_decoded dec = Aldo_Decode[opcode];
    int instlen = InstLens[dec.mode];
    if ((size_t)instlen > bv->size - at) return ALDO_DIS_ERR_EOF;

    *parsed = (struct aldo_dis_instruction){
        at,
        {bv->ord, (size_t)instlen, bv->mem + at},
        dec,
    };
    return instlen;
}

int aldo_dis_parsemem_inst(size_t size, const uint8_t mem[restrict size],
                           size_t at, struct aldo_dis_instruction *parsed)
{
    struct aldo_blockview bv = {
        .mem = mem,
        .size = size,
    };
    return aldo_dis_parse_inst(&bv, at, parsed);
}

int aldo_dis_inst(uint16_t addr, const struct aldo_dis_instruction *inst,
                  char dis[restrict static ALDO_DIS_INST_SIZE])
{
    assert(inst != NULL);
    assert(dis != NULL);

    if (!inst->bv.mem) {
        dis[0] = '\0';
        return 0;
    }

    int count, total;
    total = count = print_raw(addr, inst, dis);
    if (count < 0) return count;

    // NOTE: padding between raw bytes and disassembled instruction
    count = sprintf(dis + total, "%*s", ((3 - (int)inst->bv.size) * 3) + 1,
                    "");
    if (count < 0) return ALDO_DIS_ERR_FMT;
    total += count;

    count = print_instruction(inst, dis + total);
    if (count < 0) return count;
    if (inst->d.unofficial && total > 0) {
        dis[total - 1] = '*';
    }
    total += count;

    assert((unsigned int)total < ALDO_DIS_INST_SIZE);
    return total;
}

int aldo_dis_datapath(const struct aldo_snapshot *snp,
                      char dis[restrict static ALDO_DIS_DATAP_SIZE])
{
    assert(snp != NULL);
    assert(dis != NULL);

    struct aldo_dis_instruction inst;
    int err = aldo_dis_parsemem_inst(snp->prg.curr->length, snp->prg.curr->pc,
                                     0, &inst);
    if (err < 0) return err;

    // NOTE: we're in an interrupt state so adjust the instruction to render
    // the datapath correctly.
    if (snp->cpu.datapath.opcode == Aldo_BrkOpcode
        && inst.d.instruction != ALDO_IN_BRK) {
        inst.d = Aldo_Decode[snp->cpu.datapath.opcode];
        inst.bv.size = (size_t)InstLens[inst.d.mode];
    }

    int count, total;
    total = count = sprintf(dis, "%s ", mnemonic(inst.d.instruction));
    if (count < 0) return ALDO_DIS_ERR_FMT;

    size_t
        max_offset = 1 + (inst.bv.size / 3),
        displayidx = (size_t)snp->cpu.datapath.exec_cycle < max_offset
                        ? (size_t)snp->cpu.datapath.exec_cycle
                        : max_offset;
    const char *displaystr = StringTables[inst.d.mode][displayidx];
    switch (displayidx) {
    case 0:
        count = sprintf(dis + total, "%s", displaystr);
        break;
    case 1:
        count = snp->cpu.datapath.opcode == Aldo_BrkOpcode
                    ? sprintf(dis + total, displaystr, interrupt_display(snp))
                    : sprintf(dis + total, displaystr,
                              strlen(displaystr) > 0 ? inst.bv.mem[1] : 0);
        break;
    default:
        count = sprintf(dis + total, displaystr,
                        strlen(displaystr) > 0
                        ? aldo_batowr(inst.bv.mem + 1)
                        : 0);
        break;
    }
    if (count < 0) return ALDO_DIS_ERR_FMT;
    total += count;

    assert((unsigned int)total < ALDO_DIS_DATAP_SIZE);
    return total;
}

int aldo_dis_cart_prg(aldo_cart *cart, const char *restrict name, bool verbose,
                      bool unified_output, FILE *f)
{
    assert(cart != NULL);
    assert(name != NULL);
    assert(f != NULL);

    struct aldo_blockview bv = aldo_cart_prgblock(cart, 0);
    if (!bv.mem) return ALDO_DIS_ERR_PRGROM;

    if (aldo_cart_write_dis_header(cart, name, f) < 0) return ALDO_DIS_ERR_IO;

    do {
        if (fputc('\n', f) == EOF) return ALDO_DIS_ERR_IO;
        int err = print_prgblock(&bv, verbose, f);
        // NOTE: disassembly errors may occur normally if data bytes are
        // interpreted as instructions so note the result and continue.
        if (err < 0) {
            err = fprintf(unified_output ? f : stderr,
                          "Dis err (%d): %s\n", err, aldo_dis_errstr(err));
            if (err < 0) return ALDO_DIS_ERR_IO;
        }
        bv = aldo_cart_prgblock(cart, bv.ord + 1);
    } while (bv.mem);

    return 0;
}

int aldo_dis_cart_chr(aldo_cart *cart, int chrscale,
                      const char *restrict chrdecode_prefix, FILE *output)
{
    assert(cart != NULL);
    assert(output != NULL);

    if (chrscale <= 0 || chrscale > ScaleGuard) return ALDO_DIS_ERR_CHRSCL;

    struct aldo_blockview bv = aldo_cart_chrblock(cart, 0);
    if (!bv.mem) return ALDO_DIS_ERR_CHRROM;

    const char *prefix = chrdecode_prefix && chrdecode_prefix[0] != '\0'
                            ? chrdecode_prefix
                            : "chr";
    size_t
        prefixlen = strlen(prefix),
        namesize = prefixlen + 8;   // NOTE: prefix + nnn.bmp + nul
    char *bmpfilename = malloc(namesize);
    if (!bmpfilename) return ALDO_DIS_ERR_ERNO;

    int err;
    do {
        err = snprintf(bmpfilename, namesize, "%s%03zu.bmp", prefix, bv.ord);
        if (err < 0) {
            err = ALDO_DIS_ERR_FMT;
            break;
        }
        err = write_chrblock(&bv, (uint32_t)chrscale, bmpfilename, output);
        if (err < 0) break;
        bv = aldo_cart_chrblock(cart, bv.ord + 1);
    } while (bv.mem);

    free(bmpfilename);
    return err;
}

int aldo_dis_cart_chrblock(const struct aldo_blockview *bv, int scale, FILE *f)
{
    assert(bv != NULL);
    assert(f != NULL);

    if (!bv->mem) return ALDO_DIS_ERR_CHRROM;
    if (scale <= 0 || scale > ScaleGuard) return ALDO_DIS_ERR_CHRSCL;

    uint32_t tilesdim, tile_sections;
    int err = measure_tile_sheet(bv->size, &tilesdim, &tile_sections);
    if (err < 0) return err;

    return write_chrtiles(bv, tilesdim, tile_sections, (uint32_t)scale, f);
}

const char *aldo_dis_inst_mnemonic(const struct aldo_dis_instruction *inst)
{
    assert(inst != NULL);

    return mnemonic(inst->d.instruction);
}

const char *aldo_dis_inst_description(const struct aldo_dis_instruction *inst)
{
    assert(inst != NULL);

    return description(inst->d.instruction);
}

const char *aldo_dis_inst_addrmode(const struct aldo_dis_instruction *inst)
{
    assert(inst != NULL);

    return modename(inst->d.mode);
}

uint8_t aldo_dis_inst_flags(const struct aldo_dis_instruction *inst)
{
    assert(inst != NULL);

    return flags(inst->d.instruction);
}

int aldo_dis_inst_operand(const struct aldo_dis_instruction *inst,
                          char dis[restrict static ALDO_DIS_OPERAND_SIZE])
{
    assert(inst != NULL);
    assert(dis != NULL);

    if (!inst->bv.mem) {
        dis[0] = '\0';
        return 0;
    }

    int count = print_operand(inst, dis);

    assert((unsigned int)count < ALDO_DIS_OPERAND_SIZE);
    return count;
}

bool aldo_dis_inst_equal(const struct aldo_dis_instruction *lhs,
                         const struct aldo_dis_instruction *rhs)
{
    return lhs && rhs && lhs->bv.size == rhs->bv.size
            && memcmp(lhs->bv.mem, rhs->bv.mem, lhs->bv.size) == 0;
}

//
// MARK: - Internal Interface
//

int aldo_dis_peek(struct aldo_mos6502 *cpu, struct aldo_rp2c02 *ppu,
                  aldo_debugger *dbg, const struct aldo_snapshot *snp,
                  char dis[restrict static ALDO_DIS_PEEK_SIZE])
{
    assert(cpu != NULL);
    assert(ppu != NULL);
    assert(dbg != NULL);
    assert(snp != NULL);
    assert(dis != NULL);

    int total = 0;
    const char *interrupt = interrupt_display(snp);
    if (strlen(interrupt) > 0) {
        int count = total = sprintf(dis, "%s > ", interrupt);
        if (count < 0) return ALDO_DIS_ERR_FMT;
        const char *fmt;
        uint16_t vector;
        int resetvector;
        if (snp->cpu.datapath.rst == ALDO_SIG_COMMITTED
            && (resetvector = aldo_debug_vector_override(dbg))
                != Aldo_NoResetVector) {
            fmt = ALDO_HEXPR_RST_IND "%04X";
            vector = (uint16_t)resetvector;
        } else {
            fmt = "%04X";
            vector = interrupt_vector(snp);
        }
        count = sprintf(dis + total, fmt, vector);
        if (count < 0) return ALDO_DIS_ERR_FMT;
        total += count;
    } else {
        struct aldo_peekresult peek = run_peek(cpu, ppu);
        switch (peek.mode) {
#define XPEEK(...) sprintf(dis, __VA_ARGS__)
#define X(s, b, n, p, ...) case ALDO_AM_LBL(s): total = p; break;
            ALDO_DEC_ADDRMODE_X
#undef X
#undef XPEEK
        default:
            assert(((void)"BAD ADDRMODE PEEK", false));
            return ALDO_DIS_ERR_INV_ADDRMD;
        }
        if (total < 0) return ALDO_DIS_ERR_FMT;
        if (peek.busfault) {
            char *data = strrchr(dis, '=');
            if (data) {
                // NOTE: verify last '=' leaves enough space for "FLT"
                assert(dis + ALDO_DIS_PEEK_SIZE - data >= 6);
                strcpy(data + 2, "FLT");
                ++total;
            }
        }
    }

    assert((unsigned int)total < ALDO_DIS_PEEK_SIZE);
    return total;
}
