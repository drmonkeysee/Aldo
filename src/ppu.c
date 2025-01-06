//
//  ppu.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ppu.h"

#include "bus.h"
#include "snapshot.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#define memclr(mem) memset(mem, 0, sizeof (mem) / sizeof (mem)[0])
#define memdump(mem, f) \
(fwrite(mem, sizeof (mem)[0], sizeof (mem) / sizeof (mem)[0], f) \
== sizeof (mem) / sizeof (mem)[0])

// NOTE: a single NTSC frame is 262 scanlines of 341 dots, counting h-blank,
// v-blank, overscan, etc; nominally 1 frame is 262 * 341 = 89342 ppu cycles;
// however with rendering enabled the odd frames skip a dot for an overall
// average cycle count of 89341.5 per frame.
static const int
    Dots = 341, Lines = 262,
    LinePostRender = 240, LineVBlank = LinePostRender + 1,
    LinePreRender = Lines - 1,
    DotPxStart = 2, DotSpriteFetch = 257, DotPxEnd = 260,
    DotTilePrefetch = 321, DotTilePrefetchEnd = 337;

// NOTE: helpers for manipulating v and t registers
static const uint16_t
    BaseNtAddr = 0x2000, HNtBit = 0x400, VNtBit = 0x800,
    NtBits = VNtBit | HNtBit, CourseYBits = 0x3e0, FineYBits = 0x7000;
static const uint8_t CourseXBits = 0x1f, PaletteMask = CourseXBits;

//
// MARK: - Registers
//

static uint8_t get_ctrl(const struct aldo_rp2c02 *self)
{
    return (uint8_t)
        (self->ctrl.nl
         | self->ctrl.nh << 1
         | self->ctrl.i << 2
         | self->ctrl.s << 3
         | self->ctrl.b << 4
         | self->ctrl.h << 5
         // NOTE: skip P
         | self->ctrl.v << 7);
}

static void set_ctrl(struct aldo_rp2c02 *self, uint8_t v)
{
    self->ctrl.nl = v & 0x1;
    self->ctrl.nh = v & 0x2;
    self->ctrl.i = v & 0x4;
    self->ctrl.s = v & 0x8;
    self->ctrl.b = v & 0x10;
    self->ctrl.h = v & 0x20;
    // NOTE: P is always grounded
    self->ctrl.v = v & 0x80;
}

static uint8_t get_mask(const struct aldo_rp2c02 *self)
{
    return (uint8_t)
        (self->mask.g
         | self->mask.bm << 1
         | self->mask.sm << 2
         | self->mask.b << 3
         | self->mask.s << 4
         | self->mask.re << 5
         | self->mask.ge << 6
         | self->mask.be << 7);
}

static void set_mask(struct aldo_rp2c02 *self, uint8_t v)
{
    self->mask.g = v & 0x1;
    self->mask.bm = v & 0x2;
    self->mask.sm = v & 0x4;
    self->mask.b = v & 0x8;
    self->mask.s = v & 0x10;
    self->mask.re = v & 0x20;
    self->mask.ge = v & 0x40;
    self->mask.be = v & 0x80;
}

static uint8_t get_status(const struct aldo_rp2c02 *self)
{
    return (uint8_t)
        (self->status.o << 5 | self->status.s << 6 | self->status.v << 7);
}

static void set_status(struct aldo_rp2c02 *self, uint8_t v)
{
    self->status.o = v & 0x20;
    self->status.s = v & 0x40;
    self->status.v = v & 0x80;
}

// NOTE: address bus is 14 bits wide
static uint16_t maskaddr(uint16_t addr)
{
    return addr & ALDO_ADDRMASK_16KB;
}

static bool rendering_disabled(const struct aldo_rp2c02 *self)
{
    return !self->mask.b && !self->mask.s;
}

static bool tile_rendering(const struct aldo_rp2c02 *self)
{
    return (0 < self->dot && self->dot < DotSpriteFetch)
            || (DotTilePrefetch <= self->dot
                && self->dot < DotTilePrefetchEnd);
}

static bool sprite_fetch(const struct aldo_rp2c02 *self)
{
    return DotSpriteFetch <= self->dot && self->dot < DotTilePrefetch;
}

static bool in_postrender(const struct aldo_rp2c02 *self)
{
    return LinePostRender <= self->line && self->line < LinePreRender;
}

static bool in_vblank(const struct aldo_rp2c02 *self)
{
    return (self->line == LineVBlank && self->dot >= 1)
            || (LineVBlank < self->line && self->line < LinePreRender)
            || (self->line == LinePreRender && self->dot == 0);
}

//
// MARK: - Palette
//

static bool palette_addr(uint16_t addr)
{
    // NOTE: addr=[$3F00-$3FFF]
    return Aldo_PaletteStartAddr <= addr && addr < ALDO_MEMBLOCK_16KB;
}

static uint16_t mask_palette(uint16_t addr)
{
    // NOTE: 32 addressable bytes, including mirrors
    addr &= PaletteMask;
    // NOTE: background has 16 allocated slots while sprites have 12, making
    // palette RAM only 28 bytes long; if the address points at a mirrored slot
    // then mask it down to the actual slots in the lower 16, otherwise adjust
    // the palette address down to the "real" sprite slot, accounting for the
    // 4 missing entries at $3F10, $3F14, $3F18, and $3F1C.
    if (addr & 0x10) {
        if ((addr & 0x3) == 0) {
            addr &= 0xf;
        } else {
            // NOTE: 0x1-0x3 adjust down 1, 0x5-0x7 adjust down 2, etc...
            addr -= (uint16_t)(((addr & 0xc) >> 2) + 1);
        }
    }
    return addr;
}

static uint8_t palette_read(const struct aldo_rp2c02 *self, uint16_t addr)
{
    // NOTE: addr=[$3F00-$3FFF]
    assert(palette_addr(addr));

    uint8_t pal = self->palette[mask_palette(addr)];
    // NOTE: palette values are 6 bits wide, grayscale additionally masks down
    // to lowest palette column.
    return pal & (self->mask.g ? 0x30 : 0x3f);
}

static void palette_write(struct aldo_rp2c02 *self, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$3F00-$3FFF]
    assert(palette_addr(addr));

    self->palette[mask_palette(addr)] = d;
}

static void
snapshot_palette(const struct aldo_rp2c02 *self,
                 uint8_t palsnp[static ALDO_PAL_SIZE][ALDO_PAL_SIZE],
                 uint16_t offset)
{
    uint16_t base = Aldo_PaletteStartAddr + offset;
    for (size_t i = 0; i < ALDO_PAL_SIZE; ++i) {
        uint8_t *p = palsnp[i];
        uint16_t addr = base + (uint16_t)(ALDO_PAL_SIZE * i);
        // NOTE: 1st color is always the backdrop
        p[0] = palette_read(self, base);
        p[1] = palette_read(self, addr + 1);
        p[2] = palette_read(self, addr + 2);
        p[3] = palette_read(self, addr + 3);
    }
}

//
// MARK: - Main Bus Device (PPU registers)
//

static bool regread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$2000-$3FFF]
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    struct aldo_rp2c02 *ppu = ctx;
    ppu->signal.rw = true;
    ppu->regsel = addr & 0x7;
    switch (ppu->regsel) {
    case 2: // PPUSTATUS
        // NOTE: NMI race condition; if status is read just before NMI is
        // fired, then vblank is cleared but returns false and NMI is missed.
        if (ppu->line == LineVBlank && ppu->dot == 1) {
            ppu->status.v = false;
        }
        ppu->regbus = (ppu->regbus & 0x1f) | get_status(ppu);
        ppu->w = ppu->status.v = false;
        break;
    case 4: // OAMDATA
        ppu->regbus = ppu->oam[ppu->oamaddr];
        break;
    case 7: // PPUDATA
        ppu->cvp = true;
        uint16_t addr = maskaddr(ppu->v);
        ppu->regbus = palette_addr(addr) ? palette_read(ppu, addr) : ppu->rbuf;
        break;
    default:
        break;
    }
    *d = ppu->regbus;
    return true;
}

static bool regwrite(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$2000-$3FFF]
    assert(ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB);

    struct aldo_rp2c02 *ppu = ctx;
    ppu->signal.rw = false;
    ppu->regsel = addr & 0x7;
    ppu->regbus = d;
    switch (ppu->regsel) {
    case 0: // PPUCTRL
        if (ppu->rst != ALDO_SIG_SERVICED) {
            set_ctrl(ppu, ppu->regbus);
            // NOTE: set nametable-select
            ppu->t = (uint16_t)((ppu->t & ~NtBits) | ppu->ctrl.nh << 11
                                | ppu->ctrl.nl << 10);
            // TODO: bit 0 race condition on dot 257
        }
        break;
    case 1: // PPUMASK
        if (ppu->rst != ALDO_SIG_SERVICED) {
            set_mask(ppu, ppu->regbus);
        }
        break;
    case 3: // OAMADDR
        ppu->oamaddr = ppu->regbus;
        // TODO: there are some OAM corruption effects here that I don't
        // understand yet.
        break;
    case 4: // OAMDATA
        // TODO: this logic is shared by OAMDMA
        if (in_postrender(ppu) || rendering_disabled(ppu)) {
            ppu->oam[ppu->oamaddr++] = ppu->regbus;
        } else {
            // NOTE: during rendering, writing to OAMDATA does not change OAM
            // but it does increment oamaddr by one object attribute (4-bytes),
            // corrupting sprite evaluation.
            ppu->oamaddr += 0x4;
        }
        break;
    case 5: // PPUSCROLL
        if (ppu->rst != ALDO_SIG_SERVICED) {
            static const uint8_t course = 0xf8, fine = 0x7;
            if (ppu->w) {
                // NOTE: set course and fine y
                ppu->t = (uint16_t)((ppu->t & ~(FineYBits | CourseYBits))
                                    | (ppu->regbus & fine) << 12
                                    | (ppu->regbus & course) << 2);
            } else {
                // NOTE: set course and fine x
                ppu->t = (uint16_t)((ppu->t & ~CourseXBits)
                                    | (ppu->regbus & course) >> 3);
                ppu->x = ppu->regbus & fine;
            }
            ppu->w = !ppu->w;
        }
        break;
    case 6: // PPUADDR
        if (ppu->rst != ALDO_SIG_SERVICED) {
            if (ppu->w) {
                ppu->t = (ppu->t & 0x7f00) | ppu->regbus;
                ppu->v = ppu->t;
            } else {
                ppu->t = aldo_bytowr((uint8_t)ppu->t, ppu->regbus & 0x3f);
            }
            ppu->w = !ppu->w;
            // TODO: there is some kind of bus conflict behavior i don't
            // understand yet, as well as palette corruption.
        }
        break;
    case 7: // PPUDATA
        ppu->cvp = true;
        uint16_t addr = maskaddr(ppu->v);
        if (palette_addr(addr)) {
            palette_write(ppu, addr, ppu->regbus);
        }
        break;
    default:
        break;
    }
    return true;
}

//
// MARK: - PPU Bus Device (Pattern Tables, Nametables, Palette)
//

static void addrbus(struct aldo_rp2c02 *self, uint16_t addr)
{
    self->vaddrbus = maskaddr(addr);
    self->signal.ale = true;
}

static void read(struct aldo_rp2c02 *self)
{
    // NOTE: addr=[$0000-$3FFF]
    assert(self->vaddrbus < ALDO_MEMBLOCK_16KB);

    self->signal.ale = self->signal.rd = false;
    self->bflt = !aldo_bus_read(self->vbus, self->vaddrbus, &self->vdatabus);
}

static void write(struct aldo_rp2c02 *self)
{
    // NOTE: addr=[$0000-$3FFF]
    assert(self->vaddrbus < ALDO_MEMBLOCK_16KB);

    self->signal.ale = false;
    self->vdatabus = self->regbus;
    if (palette_addr(self->vaddrbus)) {
        self->bflt = false;
    } else {
        self->signal.wr = false;
        self->bflt = !aldo_bus_write(self->vbus, self->vaddrbus,
                                     self->vdatabus);
    }
}

static void snapshot_nametables(const struct aldo_rp2c02 *self,
                                struct aldo_snapshot *snp)
{
    snp->video->nt.pt = self->ctrl.b;
    // then calculate palette ids
    for (size_t i = 0;
         i < sizeof snp->video->nt.tables / sizeof snp->video->nt.tables[0];
         ++i) {
        uint16_t base_addr = BaseNtAddr + (HNtBit * (uint16_t)i);
        size_t
            tile_count = sizeof snp->video->nt.tables[i].tiles
                            / sizeof snp->video->nt.tables[i].tiles[0],
            attr_count = sizeof snp->video->nt.tables[i].attributes
                            / sizeof snp->video->nt.tables[i].attributes[0];
        aldo_bus_copy(self->vbus, base_addr, tile_count,
                      snp->video->nt.tables[i].tiles);
        aldo_bus_copy(self->vbus, base_addr + (uint16_t)tile_count, attr_count,
                      snp->video->nt.tables[i].attributes);
    }
}

//
// MARK: - Rendering
//

/*
 * v register composition for background tile fetches, based mostly on:
 *  https://www.nesdev.org/wiki/PPU_scrolling
 *  https://www.nesdev.org/wiki/PPU_pattern_tables

 * v composition during rendering
 * yyy NN YYYYY XXXXX
 * ||| || ||||| +++++-- coarse X scroll (32 tiles)
 * ||| || +++++-------- coarse Y scroll (30 rows + attributes)
 * ||| ++-------------- nametable select (4 nametables)
 * +++----------------- fine Y scroll (ignored)

 * Fetch address definitions
 * tile address = 0x2000 | (v & 0x0FFF)
 * attribute address = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)

 * Attribute Fetch
 *  NN 1111 YYY XXX
 *  || |||| ||| +++-- high 3 bits of coarse X (x/4) (shifted and masked)
 *  || |||| +++------ high 3 bits of coarse Y (y/4) (shifted and masked)
 *  || ++++---------- attribute offset (960 bytes) (23C0)
 *  ++--------------- nametable select (masked)

 * Pattern Fetch
 * DCBA98 76543210
 * ---------------
 * 0HNNNN NNNNPyyy
 * |||||| |||||+++- T: Fine Y offset, the row number within a tile
 * |||||| ||||+---- P: Bit plane (0: less significant bit; 1: more significant bit)
 * ||++++-++++----- N: Tile number from name table
 * |+-------------- H: Half of pattern table (0: "left"; 1: "right")
 * +--------------- 0: Pattern table is at $0000-$1FFF
 */

static void latch_pattern(uint16_t *latch, uint8_t val)
{
    *latch = (*latch & 0xff00) | val;
}

/*
 * Attribute bit selection for pixel pipeline, taken mostly from:
 * https://www.nesdev.org/wiki/PPU_attribute_tables
 *
 * Attribute table is 64 bytes arranged in the same 32x30 tile layout as the
 * Nametables. An Attribute byte defines palette selection for 4x4 tiles
 * (32x32 pixels), providing 4 possible palette options per each 2x2 tile
 * (16x16 pixel) quadrant (2-bits):
 *
 * 7654 3210
 * |||| ||++- Color bits 3-2 for top left quadrant of this byte
 * |||| ++--- Color bits 3-2 for top right quadrant of this byte
 * ||++------ Color bits 3-2 for bottom left quadrant of this byte
 * ++-------- Color bits 3-2 for bottom right quadrant of this byte
 *
 * This means each quadrant starts at an X-offset of 0 or 2 and a Y-offset of
 * 0 or 2 from the layout origin of the attribute; therefore the quadrant
 * selection is determined by the 1st bit of course-x and course-y (recall
 * course-x and course-y define the coordinates of each 8x8 tile).
 *
 * ,---+---+---+---.
 * | X0,Y0 | X2,Y0 |
 * +---+---+---+---+
 * | X0,Y2 | X2,Y2 |
 * `---+---+---+---'
 *
 * Since each quadrant is 2-bits the x,y coords select one of the 0th, 2nd,
 * 4th, or 6th bits as the low-bit, the high-bit is the neighboring odd-bit.
 */

// NOTE: tile x-increment happens one dot before the new tile is latched into
// the shift registers, so we need to determine the attribute byte quadrant
// before the increment and remember it for the subsequent latch dot; otherwise
// some of the tile attributes for the 4x4 metatile will be paired off with the
// wrong quadrant bits due to erroneously looking one x-tile ahead.
static void lock_attribute(struct aldo_rp2c02 *self)
{
    self->pxpl.atb = (self->v & 0x2) | ((self->v & 0x40) >> 4);
}

static void latch_attribute(struct aldo_rp2c02 *self)
{
    self->pxpl.atl[0] = aldo_getbit(self->pxpl.at, self->pxpl.atb);
    self->pxpl.atl[1] = aldo_getbit(self->pxpl.at, self->pxpl.atb + 1);
}

static void latch_tile(struct aldo_rp2c02 *self)
{
    latch_pattern(self->pxpl.bgs, self->pxpl.bg[0]);
    latch_pattern(self->pxpl.bgs + 1, self->pxpl.bg[1]);
    latch_attribute(self);
}

static void mux_bg(struct aldo_rp2c02 *self)
{
    static const int left_mask_end = DotPxStart + 8;

    // NOTE: fine-x selects bit from the left: 0 = 7th bit, 7 = 0th bit
    int abit = 7 - self->x;
    self->pxpl.mux = (uint8_t)((aldo_getbit(self->pxpl.ats[1], abit) << 3)
                               | (aldo_getbit(self->pxpl.ats[0], abit) << 2));
    if (self->mask.b && (self->mask.bm || self->dot >= left_mask_end)) {
        // NOTE: tile selection is from the left-most (upper) byte
        int tbit = abit + 8;
        self->pxpl.mux |= (uint8_t)((aldo_getbit(self->pxpl.bgs[1], tbit) << 1)
                                    | aldo_getbit(self->pxpl.bgs[0], tbit));
    }
    assert(self->pxpl.mux < 0x10);
}

static void mux_fg(struct aldo_rp2c02 *self)
{
    (void)self;
    // TODO: select sprite priority here
    assert(self->pxpl.mux < 0x10);
}

static void resolve_palette(struct aldo_rp2c02 *self)
{
    if (rendering_disabled(self)) {
        // NOTE: if v is pointing into palette memory then render that color
        if (palette_addr(self->v)) {
            self->pxpl.pal = (uint8_t)(self->v & PaletteMask);
        } else {
            self->pxpl.pal = 0x0;
        }
    } else if ((self->pxpl.mux & 0x3) == 0) {
        // NOTE: transparent pixels fall through to backdrop color
        self->pxpl.pal = 0x0;
    } else {
        self->pxpl.pal = self->pxpl.mux;
    }
}

static void output_pixel(struct aldo_rp2c02 *self)
{
    assert(self->pxpl.pal < 0x20);

    uint16_t pal_addr = Aldo_PaletteStartAddr | self->pxpl.pal;
    self->pxpl.px = palette_read(self, pal_addr);
    self->signal.vout = true;
}

// TODO: replace t with c23 typeof
#define pxshift(t, r, v) (*(r) = (t)(*(r) << 1) | (v))

static void shift_tiles(struct aldo_rp2c02 *self)
{
    pxshift(uint16_t, self->pxpl.bgs, 1);
    pxshift(uint8_t, self->pxpl.ats, self->pxpl.atl[0]);
    pxshift(uint16_t, self->pxpl.bgs + 1, 1);
    pxshift(uint8_t, self->pxpl.ats + 1, self->pxpl.atl[1]);
    if (self->dot % 8 == 1) {
        latch_tile(self);
    }
}

static uint16_t nametable_addr(const struct aldo_rp2c02 *self)
{
    return BaseNtAddr | (self->v & ALDO_ADDRMASK_4KB);
}

static uint16_t pattern_addr(const struct aldo_rp2c02 *self, bool table,
                             bool plane)
{
    int tileidx = self->pxpl.nt << 4, pxrow = (self->v & FineYBits) >> 12;
    return (uint16_t)((table << 12) | tileidx | (plane << 3) | pxrow);
}

static void read_nt(struct aldo_rp2c02 *self)
{
    read(self);
    self->pxpl.nt = self->vdatabus;
}

static void incr_course_x(struct aldo_rp2c02 *self)
{
    // NOTE: wraparound at x = 32, overflow to horizontal nametable bit
    if ((self->v & CourseXBits) == CourseXBits) {
        self->v &= (uint16_t)~CourseXBits;
        self->v ^= HNtBit;
    } else {
        self->v += 1;
    }
}

static void incr_y(struct aldo_rp2c02 *self)
{
    static const uint16_t max_course_y = 29 << 5;

    // NOTE: fine y wraparound at y = 8, overflow into course y;
    // course y wraparound at y = 30, overflow into vertical nametable bit;
    // if course y > 29 then wraparound occurs without overflow.
    if ((self->v & FineYBits) == FineYBits) {
        self->v &= (uint16_t)~FineYBits;
        uint16_t course_y = self->v & CourseYBits;
        if (course_y >= max_course_y) {
            self->v &= (uint16_t)~CourseYBits;
            if (course_y == max_course_y) {
                self->v ^= VNtBit;
            }
        } else {
            self->v += 0x20;
        }
    } else {
        self->v += 0x1000;
    }
}

static void increment_tile(struct aldo_rp2c02 *self)
{
    lock_attribute(self);
    incr_course_x(self);
    if (self->dot == DotSpriteFetch - 1) {
        incr_y(self);
    }
}

// NOTE: based on PPU diagram: https://www.nesdev.org/wiki/PPU_rendering
static void pixel_pipeline(struct aldo_rp2c02 *self)
{
    // NOTE: assume there is no video signal until we actually output a pixel
    self->signal.vout = false;

    if (in_postrender(self)) return;

    if (DotPxStart <= self->dot && self->dot < DotPxEnd
        && self->line != LinePreRender) {
        if (self->dot > DotPxStart + 1) {
            output_pixel(self);
        }
        if (self->dot > DotPxStart) {
            resolve_palette(self);
        }
        mux_bg(self);
        mux_fg(self);
        shift_tiles(self);
        // TODO: shift sprites
    } else if (self->dot == 329) {
        // NOTE: prefetch likely runs the same hardware steps as normal pixel
        // selection, but since there are no side-effects outside of the pixel
        // pipeline it's easier to load the tiles at once.
        latch_tile(self);
        // NOTE: shift the first tile that occurs during dots 329-337
        self->pxpl.bgs[0] <<= 8;
        self->pxpl.ats[0] = -self->pxpl.atl[0];
        self->pxpl.bgs[1] <<= 8;
        self->pxpl.ats[1] = -self->pxpl.atl[1];
    } else if (self->dot == 337) {
        latch_tile(self);
    }
}

static void tile_read(struct aldo_rp2c02 *self)
{
    switch (self->dot % 8) {
    case 1:
        // NT addr
        addrbus(self, nametable_addr(self));
        break;
    case 2:
        // NT data
        read_nt(self);
        break;
    case 3:
        // AT addr
        {
            uint16_t
                ntselect = self->v & NtBits,
                metatiley = (self->v & 0x380) >> 4,
                metatilex = (self->v & 0x1c) >> 2;
            addrbus(self, 0x23c0 | ntselect | metatiley | metatilex);
        }
        break;
    case 4:
        // AT data
        read(self);
        self->pxpl.at = self->vdatabus;
        break;
    case 5:
        // BG low addr
        addrbus(self, pattern_addr(self, self->ctrl.b, 0));
        break;
    case 6:
        // BG low data
        read(self);
        self->pxpl.bg[0] = self->vdatabus;
        break;
    case 7:
        // BG high addr
        addrbus(self, pattern_addr(self, self->ctrl.b, 1));
        break;
    case 0:
        // BG high data
        read(self);
        self->pxpl.bg[1] = self->vdatabus;
        increment_tile(self);
        break;
    default:
        assert(((void)"TILE RENDER UNREACHABLE CASE", false));
        break;
    }
}

static void sprite_read(struct aldo_rp2c02 *self)
{
    // NOTE: copy t course-y, fine-y, and vertical nametable to v
    if (self->line == LinePreRender && 280 <= self->dot && self->dot < 305) {
        static const uint16_t vert_bits = FineYBits | VNtBit | CourseYBits;
        self->v = (uint16_t)((self->v & ~vert_bits) | (self->t & vert_bits));
    }

    switch (self->dot % 8) {
    case 1:
        // garbage NT addr
        addrbus(self, nametable_addr(self));
        if (self->dot == DotSpriteFetch) {
            static const uint16_t horiz_bits = HNtBit | CourseXBits;
            // NOTE: copy t course-x and horizontal nametable to v
            self->v = (uint16_t)((self->v & ~horiz_bits)
                                 | (self->t & horiz_bits));
        }
        break;
    case 2:
        // garbage NT data
        read_nt(self);
        break;
    case 3:
        // garbage NT addr
        addrbus(self, nametable_addr(self));
        // TODO: load sprite attribute
        break;
    case 4:
        // garbage NT data
        read_nt(self);
        // TODO: load sprite x
        break;
    case 5:
        // TODO: FG low addr
        break;
    case 6:
        // TODO: FG low data
        break;
    case 7:
        // TODO: FG high addr
        break;
    case 0:
        // TODO: FG high data
        break;
    default:
        assert(((void)"SPRITE RENDER UNREACHABLE CASE", false));
        break;
    }
}

//
// MARK: - Other Internal Operations
//

static bool nextdot(struct aldo_rp2c02 *self)
{
    // NOTE: skip the last dot on odd frames if rendering is enabled
    if (self->line == LinePreRender && self->dot == Dots - 2 && self->odd
        && !rendering_disabled(self)) {
        ++self->dot;
    }
    if (++self->dot >= Dots) {
        self->dot = 0;
        if (++self->line >= Lines) {
            self->line = 0;
            self->odd = !self->odd;
            return true;
        }
    }
    return false;
}

static void reset_internals(struct aldo_rp2c02 *self)
{
    // NOTE: t is cleared but NOT v
    self->dot = self->line = self->t = self->rbuf = self->x = 0;
    self->signal.intr = true;
    self->signal.ale = self->cvp = self->odd = self->w = false;
    set_ctrl(self, 0);
    set_mask(self, 0);
}

static void reset(struct aldo_rp2c02 *self)
{
    reset_internals(self);
    self->rst = ALDO_SIG_SERVICED;
}

// NOTE: this follows the same steps as CPU reset sequence and has the same
// halting effect while rst line is held low, to keep the chips in sync.
static bool reset_held(struct aldo_rp2c02 *self)
{
    // NOTE: pending always proceeds to committed just like the CPU sequence
    if (self->rst == ALDO_SIG_PENDING) {
        self->rst = ALDO_SIG_COMMITTED;
    }

    if (self->signal.rst) {
        switch (self->rst) {
        case ALDO_SIG_DETECTED:
            self->rst = ALDO_SIG_CLEAR;
            break;
        case ALDO_SIG_COMMITTED:
            reset(self);
            break;
        default:
            break;
        }
    } else {
        switch (self->rst) {
        case ALDO_SIG_CLEAR:
        case ALDO_SIG_SERVICED:
            self->rst = ALDO_SIG_DETECTED;
            break;
        case ALDO_SIG_DETECTED:
            self->rst = ALDO_SIG_PENDING;
            break;
        case ALDO_SIG_COMMITTED:
            // NOTE: halt PPU execution as long as reset is held
            return true;
        default:
            break;
        }
    }
    return false;
}

static void vblank(struct aldo_rp2c02 *self)
{
    if (self->line == LineVBlank && self->dot == 0) {
        // NOTE: set vblank status 1 dot early to account for race-condition
        // that will suppress NMI if status.v is read (and cleared) right
        // before NMI is signaled on 241,1.
        self->status.v = true;
    } else if (in_vblank(self)) {
        // NOTE: NMI active within vblank if ctrl.v and status.v are set
        self->signal.intr = !self->ctrl.v || !self->status.v;
    } else if (self->line == LinePreRender && self->dot == 1) {
        self->signal.intr = true;
        set_status(self, 0);
        self->rst = ALDO_SIG_CLEAR;
    }
}

static void cpu_rw(struct aldo_rp2c02 *self)
{
    if (self->signal.ale) {
        if (self->signal.rw) {
            read(self);
            self->rbuf = self->vdatabus;
        } else {
            write(self);
        }
        self->cvp = false;
        self->v += (uint16_t)(1 << (self->ctrl.i * 5)); // NOTE: 1 or 32
    } else {
        addrbus(self, self->v);
    }
}

static void vram_rw(struct aldo_rp2c02 *self)
{
    if (in_postrender(self) || rendering_disabled(self)) {
        if (self->cvp) {
            cpu_rw(self);
        }
    } else if (self->dot == 0) {
        if (self->line == 0 && !self->odd) {
            read_nt(self);
        } else {
            // NOTE: BG low addr is put on bus but address latch is not
            // signaled.
            self->vaddrbus = maskaddr(pattern_addr(self, self->ctrl.b, 0));
        }
    } else if (tile_rendering(self)) {
        tile_read(self);
    } else if (sprite_fetch(self)) {
        sprite_read(self);
    } else {
        assert(DotTilePrefetchEnd <= self->dot && self->dot < Dots);
        if (self->dot % 2 == 1) {
            addrbus(self, nametable_addr(self));
        } else {
            read_nt(self);
        }
    }
}

static bool cycle(struct aldo_rp2c02 *self)
{
    // NOTE: clear any databus signals from previous cycle; unlike the CPU,
    // the PPU does not read/write on every cycle so these lines must be
    // managed explicitly.
    self->signal.rd = self->signal.wr = true;

    vblank(self);
    pixel_pipeline(self);
    vram_rw(self);

    // NOTE: dot advancement happens last, leaving PPU on next dot to be drawn;
    // analogous to stack pointer always pointing at next byte to be written.
    return nextdot(self);
}

//
// MARK: - Public Interface
//

const uint16_t Aldo_PaletteStartAddr = ALDO_MEMBLOCK_16KB - 256;
const int Aldo_DotsPerFrame = Dots * Lines, Aldo_PpuRatio = 3;

void aldo_ppu_connect(struct aldo_rp2c02 *self, aldo_bus *mbus)
{
    assert(self != NULL);
    assert(mbus != NULL);

    bool r = aldo_bus_set(mbus, ALDO_MEMBLOCK_8KB, (struct aldo_busdevice){
        .read = regread,
        .write = regwrite,
        .ctx = self,
    });
    (void)r, assert(r);
}

void aldo_ppu_powerup(struct aldo_rp2c02 *self)
{
    assert(self != NULL);
    assert(self->vbus != NULL);

    // NOTE: initialize ppu to known state
    self->oamaddr = 0;
    self->signal.rst = self->signal.rw = self->signal.rd = self->signal.wr =
        true;
    self->signal.vout = self->bflt = false;

    // NOTE: according to https://www.nesdev.org/wiki/PPU_power_up_state, the
    // vblank flag is set on powerup more often than not, but at least with
    // Aldo's timing this causes a 100% reproable visual glitch on Donkey Kong,
    // due to only checking PPUSTATUS once on startup and attempting to clear
    // the screen on the ppu's idle frame. Most other games handle this by
    // checking vblank at least twice on startup. Somehow this is not a problem
    // on real hardware, though according to
    // https://forums.nesdev.org/viewtopic.php?t=19792 it CAN be reproed on
    // some major emulators after modifying them to simulate the ppu idle frame
    // and it CAN be reproed occasionally on reset or short power-cycles on
    // real hardware.
    // So... this seems like a legitimate startup bug in Donkey Kong that, for
    // real hardware timing reasons, never seems to manifest on powerup but
    // nevertheless is *possible*.
    // I split the difference between wiki documentation and functionality by
    // clearing the status register on powerup but letting its value float
    // on reset, making it possible to occasionally recreate this (real?) bug.
    set_status(self, 0);

    reset_internals(self);

    // NOTE: simulate rst set on startup to engage reset sequence
    self->rst = ALDO_SIG_PENDING;
}

void aldo_ppu_zeroram(struct aldo_rp2c02 *self)
{
    assert(self != NULL);

    memclr(self->oam);
    memclr(self->soam);
    memclr(self->palette);
}

bool aldo_ppu_cycle(struct aldo_rp2c02 *self)
{
    assert(self != NULL);

    if (reset_held(self)) return false;
    return cycle(self);
}

void aldo_ppu_bus_snapshot(const struct aldo_rp2c02 *self,
                           struct aldo_snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);

    snp->ppu.ctrl = get_ctrl(self);
    snp->ppu.mask = get_mask(self);
    snp->ppu.oamaddr = self->oamaddr;
    snp->ppu.status = get_status(self);

    snp->ppu.pipeline.rst = self->rst;
    snp->ppu.pipeline.addressbus = self->vaddrbus;
    snp->ppu.pipeline.scrolladdr = self->v;
    snp->ppu.pipeline.tempaddr = self->t;
    snp->ppu.pipeline.dot = self->dot;
    snp->ppu.pipeline.line = self->line;
    snp->ppu.pipeline.databus = self->vdatabus;
    snp->ppu.pipeline.readbuffer = self->rbuf;
    snp->ppu.pipeline.register_databus = self->regbus;
    snp->ppu.pipeline.register_select = self->regsel;
    snp->ppu.pipeline.xfine = self->x;
    snp->ppu.pipeline.busfault = self->bflt;
    snp->ppu.pipeline.cv_pending = self->cvp;
    snp->ppu.pipeline.oddframe = self->odd;
    snp->ppu.pipeline.writelatch = self->w;
    snp->ppu.pipeline.pixel = self->pxpl.px;

    snp->ppu.lines.address_enable = self->signal.ale;
    snp->ppu.lines.cpu_readwrite = self->signal.rw;
    snp->ppu.lines.interrupt = self->signal.intr;
    snp->ppu.lines.read = self->signal.rd;
    snp->ppu.lines.reset = self->signal.rst;
    snp->ppu.lines.video_out = self->signal.vout;
    snp->ppu.lines.write = self->signal.wr;

    snp->mem.oam = self->oam;
    snp->mem.secondary_oam = self->soam;
    snp->mem.palette = self->palette;
}

void aldo_ppu_vid_snapshot(struct aldo_rp2c02 *self, struct aldo_snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);
    assert(snp->video != NULL);

    snapshot_palette(self, snp->video->palettes.bg, 0);
    snapshot_palette(self, snp->video->palettes.fg, 0x10);
    snapshot_nametables(self, snp);
}

bool aldo_ppu_dumpram(const struct aldo_rp2c02 *self, FILE *f)
{
    assert(self != NULL);
    assert(f != NULL);

    return memdump(self->oam, f)
            && memdump(self->soam, f)
            && memdump(self->palette, f);
}

struct aldo_ppu_coord aldo_ppu_trace(const struct aldo_rp2c02 *self,
                                     int adjustment)
{
    assert(self != NULL);

    struct aldo_ppu_coord c = {
        self->dot + adjustment,
        self->line,
    };
    if (c.dot < 0) {
        c.dot += Dots;
        if (--c.line < 0) {
            c.line += Lines;
        }
    }
    return c;
}

struct aldo_ppu_coord aldo_ppu_screendot(const struct aldo_rp2c02 *self)
{
    assert(self != NULL);

    static const int dot_pxout = DotPxStart + 2;

    if (!self->signal.vout) return (struct aldo_ppu_coord){-1, -1};

    // NOTE: dot will be +1 past the most recent pixel output cycle
    assert(dot_pxout < self->dot && self->dot <= DotPxEnd);
    assert(self->line < LinePostRender);

    return (struct aldo_ppu_coord){self->dot - (dot_pxout + 1), self->line};
}
