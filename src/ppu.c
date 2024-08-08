//
//  ppu.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ppu.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#define memclr(mem) memset(mem, 0, sizeof (mem) / sizeof (mem)[0]);
#define memdump(mem, f) \
fwrite(mem, sizeof (mem)[0], sizeof (mem) / sizeof (mem)[0], f);

// NOTE: a single NTSC frame is 262 scanlines of 341 dots, counting h-blank,
// v-blank, overscan, etc; nominally 1 frame is 262 * 341 = 89342 ppu cycles;
// however with rendering enabled the odd frames skip a dot for an overall
// average cycle count of 89341.5 per frame.
static const int
    Dots = 341, Lines = 262,
    LineVBlank = 241, LinePreRender = 261;

//
// MARK: - Registers
//

static uint8_t get_ctrl(const struct rp2c02 *self)
{
    return (uint8_t)
        (self->ctrl.nl
         | self->ctrl.nh << 1
         | self->ctrl.i << 2
         | self->ctrl.s << 3
         | self->ctrl.b << 4
         | self->ctrl.h << 5
         | self->ctrl.p << 6
         | self->ctrl.v << 7);
}

static void set_ctrl(struct rp2c02 *self, uint8_t v)
{
    self->ctrl.nl = v & 0x1;
    self->ctrl.nh = v & 0x2;
    self->ctrl.i = v & 0x4;
    self->ctrl.s = v & 0x8;
    self->ctrl.b = v & 0x10;
    self->ctrl.h = v & 0x20;
    self->ctrl.p = 0;           // NOTE: p is always grounded
    self->ctrl.v = v & 0x80;
}

static uint8_t get_mask(const struct rp2c02 *self)
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

static void set_mask(struct rp2c02 *self, uint8_t v)
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

static uint8_t get_status(const struct rp2c02 *self)
{
    return (uint8_t)
        (self->status.o << 5 | self->status.s << 6 | self->status.v << 7);
}

static void set_status(struct rp2c02 *self, uint8_t v)
{
    self->status.o = v & 0x20;
    self->status.s = v & 0x40;
    self->status.v = v & 0x80;
}

// NOTE: address bus is 14 bits wide
static uint16_t maskaddr(uint16_t addr)
{
    return addr & ADDRMASK_16KB;
}

static bool rendering_disabled(const struct rp2c02 *self)
{
    return !self->ctrl.b && !self->ctrl.s;
}

static bool in_postrender(const struct rp2c02 *self)
{
    static const int line_post_render = 240;

    return line_post_render <= self->line && self->line < LinePreRender;
}

static bool in_vblank(const struct rp2c02 *self)
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
    return MEMBLOCK_16KB - 256 <= addr && addr < MEMBLOCK_16KB;
}

static uint16_t mask_palette(uint16_t addr)
{
    // NOTE: 32 addressable bytes, including mirrors
    addr &= 0x1f;
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
            addr -= ((addr & 0xc) >> 2) + 1;
        }
    }
    return addr;
}

static uint8_t palette_read(const struct rp2c02 *self, uint16_t addr)
{
    // NOTE: addr=[$3F00-$3FFF]
    assert(PaletteStartAddr <= addr && addr < MEMBLOCK_16KB);

    uint8_t v = self->palette[mask_palette(addr)];
    return self->mask.g ? v & 0x30 : v;
}

static void palette_write(struct rp2c02 *self, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$3F00-$3FFF]
    assert(PaletteStartAddr <= addr && addr < MEMBLOCK_16KB);

    // NOTE: palette values are 6 bits wide
    self->palette[mask_palette(addr)] = d & 0x3f;
}

//
// MARK: - Main Bus Device (PPU registers)
//

static bool regread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$2000-$3FFF]
    assert(MEMBLOCK_8KB <= addr && addr < MEMBLOCK_16KB);

    struct rp2c02 *ppu = ctx;
    ppu->signal.rw = true;
    ppu->regsel = addr & 0x7;
    switch (ppu->regsel) {
    case 2: // PPUSTATUS
        // TODO: should this be an enum state instead
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
    assert(MEMBLOCK_8KB <= addr && addr < MEMBLOCK_16KB);

    struct rp2c02 *ppu = ctx;
    ppu->signal.rw = false;
    ppu->regsel = addr & 0x7;
    ppu->regbus = d;
    switch (ppu->regsel) {
    case 0: // PPUCTRL
        if (ppu->rst != CSGS_SERVICED) {
            set_ctrl(ppu, ppu->regbus);
            // NOTE: set nametable-select
            ppu->t = (uint16_t)((ppu->t & 0x73ff) | ppu->ctrl.nh << 11
                                | ppu->ctrl.nl << 10);
            // TODO: bit 0 race condition on dot 257
        }
        break;
    case 1: // PPUMASK
        if (ppu->rst != CSGS_SERVICED) {
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
        if (ppu->rst != CSGS_SERVICED) {
            static const uint8_t course = 0xf8, fine = 0x7;
            if (ppu->w) {
                // NOTE: set course and fine y
                ppu->t = (uint16_t)((ppu->t & 0xc1f)
                                    | (ppu->regbus & fine) << 12
                                    | (ppu->regbus & course) << 2);
            } else {
                // NOTE: set course and fine x
                ppu->t = (ppu->t & 0x7fe0) | (ppu->regbus & course) >> 3;
                ppu->x = ppu->regbus & fine;
            }
            ppu->w = !ppu->w;
        }
        break;
    case 6: // PPUADDR
        if (ppu->rst != CSGS_SERVICED) {
            if (ppu->w) {
                ppu->t = (ppu->t & 0x7f00) | ppu->regbus;
                ppu->v = ppu->t;
            } else {
                ppu->t = bytowr(ppu->t & 0xff, ppu->regbus & 0x3f);
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

static void read(struct rp2c02 *self)
{
    self->signal.ale = self->signal.rd = false;
    self->bflt = !bus_read(self->vbus, self->vaddrbus, &self->vdatabus);
    self->rbuf = self->vdatabus;
}

static void write(struct rp2c02 *self)
{
    self->signal.ale = false;
    self->vdatabus = self->regbus;
    if (palette_addr(self->vaddrbus)) {
        self->bflt = false;
    } else {
        self->signal.wr = false;
        self->bflt = !bus_write(self->vbus, self->vaddrbus, self->vdatabus);
    }
}

//
// MARK: - Internal Operations
//

static int nextdot(struct rp2c02 *self)
{
    if (++self->dot >= Dots) {
        self->dot = 0;
        if (++self->line >= Lines) {
            self->line = 0;
            self->odd = !self->odd;
            return 1;
        }
    }
    return 0;
}

static void reset(struct rp2c02 *self)
{
    // NOTE: t is cleared but NOT v
    self->dot = self->line = self->t = self->rbuf = self->x = 0;
    self->signal.intr = true;
    self->signal.vout = self->cvp = self->odd = self->w = false;
    set_ctrl(self, 0);
    set_mask(self, 0);
    self->rst = CSGS_SERVICED;
}

// NOTE: this follows the same sequence as CPU reset sequence and is checked
// only on CPU-cycle boundaries rather than each PPU cycle; this keeps the two
// chips in sync when handling the RESET signal.
static void handle_reset(struct rp2c02 *self)
{
    // NOTE: pending always proceeds to committed just like the CPU sequence
    if (self->rst == CSGS_PENDING) {
        self->rst = CSGS_COMMITTED;
    }

    if (self->signal.rst) {
        switch (self->rst) {
        case CSGS_DETECTED:
            self->rst = CSGS_CLEAR;
            break;
        case CSGS_COMMITTED:
            reset(self);
            break;
        default:
            break;
        }
    } else {
        switch (self->rst) {
        case CSGS_CLEAR:
        case CSGS_SERVICED:
            self->rst = CSGS_DETECTED;
            break;
        case CSGS_DETECTED:
            self->rst = CSGS_PENDING;
            break;
        default:
            // NOTE: as long as reset line is held low there is no further
            // effect on PPU execution.
            break;
        }
    }
}

static void vblank(struct rp2c02 *self)
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
        self->rst = CSGS_CLEAR;
    }
}

// TODO: can this be generalized to internal r/w as well?
static void cpu_rw(struct rp2c02 *self)
{
    if (self->signal.ale) {
        if (self->signal.rw) {
            read(self);
        } else {
            write(self);
        }
        self->cvp = false;
        self->v += self->ctrl.i ? 32 : 1;
    } else {
        self->vaddrbus = maskaddr(self->v);
        self->signal.ale = true;
    }
}

static int cycle(struct rp2c02 *self)
{
    // NOTE: clear any databus signals from previous cycle; unlike the CPU,
    // the PPU does not read/write on every cycle so these lines must be
    // managed explicitly.
    // TODO: does ale need clearing or is it enough to be handled on every
    // read/write?
    self->signal.rd = self->signal.wr = true;

    vblank(self);

    if (in_postrender(self) || rendering_disabled(self)) {
        if (self->cvp) {
            cpu_rw(self);
        }
    } else if (self->line == LinePreRender) {
        // TODO: prerender line
        // TODO: add odd dot skip when rendering enabled
    } else {
        // TODO: rendering
    }

    // NOTE: dot advancement happens last, leaving PPU on next dot to be drawn;
    // analogous to stack pointer always pointing at next byte to be written.
    return nextdot(self);
}

//
// MARK: - Public Interface
//

const uint16_t PaletteStartAddr = MEMBLOCK_16KB - 256;

void ppu_connect(struct rp2c02 *self, bus *mbus)
{
    assert(self != NULL);
    assert(mbus != NULL);

    bool r = bus_set(mbus, MEMBLOCK_8KB, (struct busdevice){
        .read = regread,
        .write = regwrite,
        .ctx = self,
    });
    assert(r);
}

void ppu_powerup(struct rp2c02 *self)
{
    assert(self != NULL);
    assert(self->vbus != NULL);

    // NOTE: initialize ppu to known state; internal components affected by the
    // reset sequence are deferred until that phase.
    self->regsel = self->oamaddr = 0;
    self->signal.intr = self->signal.rst = self->signal.rw = self->signal.rd =
        self->signal.wr = true;
    self->status.s = self->signal.ale = self->signal.vout = self->bflt = false;

    // NOTE: simulate rst set on startup to engage reset sequence
    self->rst = CSGS_PENDING;
}

void ppu_zeroram(struct rp2c02 *self)
{
    assert(self != NULL);

    memclr(self->oam);
    memclr(self->soam);
    memclr(self->palette);
}

int ppu_cycle(struct rp2c02 *self)
{
    assert(self != NULL);

    handle_reset(self);
    return cycle(self);
}

int ppu_frame_factor(void)
{
    return (Dots * Lines) / 3; // remove / 3
}

void ppu_snapshot(const struct rp2c02 *self, struct snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);

    snp->ppu.ctrl = get_ctrl(self);
    snp->ppu.mask = get_mask(self);
    snp->ppu.oamaddr = self->oamaddr;
    snp->ppu.status = get_status(self);

    snp->pdatapath.rst = self->rst;
    snp->pdatapath.addressbus = self->vaddrbus;
    snp->pdatapath.scrolladdr = self->v;
    snp->pdatapath.tempaddr = self->t;
    snp->pdatapath.dot = self->dot;
    snp->pdatapath.line = self->line;
    snp->pdatapath.databus = self->vdatabus;
    snp->pdatapath.readbuffer = self->rbuf;
    snp->pdatapath.register_databus = self->regbus;
    snp->pdatapath.register_select = self->regsel;
    snp->pdatapath.xfine = self->x;
    snp->pdatapath.busfault = self->bflt;
    snp->pdatapath.cv_pending = self->cvp;
    snp->pdatapath.oddframe = self->odd;
    snp->pdatapath.writelatch = self->w;

    snp->plines.address_enable = self->signal.ale;
    snp->plines.cpu_readwrite = self->signal.rw;
    snp->plines.interrupt = self->signal.intr;
    snp->plines.read = self->signal.rd;
    snp->plines.reset = self->signal.rst;
    snp->plines.video_out = self->signal.vout;
    snp->plines.write = self->signal.wr;

    snp->mem.oam = self->oam;
    snp->mem.secondary_oam = self->soam;
    snp->mem.palette = self->palette;
}

void ppu_dumpram(const struct rp2c02 *self, FILE *f)
{
    assert(self != NULL);
    assert(f != NULL);

    memdump(self->oam, f);
    memdump(self->soam, f);
    memdump(self->palette, f);
}

struct ppu_coord ppu_trace(const struct rp2c02 *self, int adjustment)
{
    assert(self != NULL);

    struct ppu_coord c = {
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
