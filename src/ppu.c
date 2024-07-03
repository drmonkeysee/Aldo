//
//  ppu.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ppu.h"

#include <assert.h>
#include <stddef.h>

// NOTE: a single NTSC frame is 262 scanlines of 341 dots, counting h-blank,
// v-blank, overscan, etc; nominally 1 frame is 262 * 341 = 89342 ppu cycles;
// however with rendering enabled the odd frames skip a dot for an overall
// average cycle count of 89341.5 per frame.
static const int
    Dots = 341, Lines = 262,
    LinePostRender = 240, LineVBlank = 241, LinePreRender = 261;

//
// Registers
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

static bool rendering_enabled(struct rp2c02 *self)
{
    return self->ctrl.b || self->ctrl.s;
}

static bool in_postrender(struct rp2c02 *self)
{
    return LinePostRender <= self->line && self->line < LinePreRender;
}

static bool in_vblank(struct rp2c02 *self)
{
    return (self->line == LineVBlank && self->dot >= 1)
            || (LineVBlank < self->line && self->line < LinePreRender)
            || (self->line == LinePreRender && self->dot == 0);
}

//
// Main Bus Device (PPU registers)
//

static bool reg_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // NOTE: addr=[$2000-$3FFF]
    assert(MEMBLOCK_8KB <= addr && addr < MEMBLOCK_16KB);

    struct rp2c02 *const ppu = ctx;
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
    default:
        break;
    }
    *d = ppu->regbus;
    return true;
}

static bool reg_write(void *ctx, uint16_t addr, uint8_t d)
{
    // NOTE: addr=[$2000-$3FFF]
    assert(MEMBLOCK_8KB <= addr && addr < MEMBLOCK_16KB);

    struct rp2c02 *const ppu = ctx;
    ppu->regsel = addr & 0x7;
    ppu->regbus = d;
    switch (ppu->regsel) {
    case 0: // PPUCTRL
        if (ppu->res != CSGS_SERVICED) {
            set_ctrl(ppu, ppu->regbus);
            // NOTE: set nametable-select
            ppu->t = (uint16_t)((ppu->t & 0x73ff) | ppu->ctrl.nh << 11
                                | ppu->ctrl.nl << 10);
            // TODO: bit 0 race condition on dot 257
        }
        break;
    case 1: // PPUMASK
        if (ppu->res != CSGS_SERVICED) {
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
        if (in_postrender(ppu) || !rendering_enabled(ppu)) {
            ppu->oam[ppu->oamaddr++] = ppu->regbus;
        } else {
            // NOTE: during rendering, writing to OAMDATA does not change OAM
            // but it does increment oamaddr by one object attribute (4-bytes),
            // corrupting sprite evaluation.
            ppu->oamaddr += 0x4;
        }
        break;
    case 5: // PPUSCROLL
        if (ppu->res != CSGS_SERVICED) {
            if (ppu->w) {
                // NOTE: set course and fine y
                ppu->t = (uint16_t)((ppu->t & 0xc1f)
                                    | (ppu->regbus & 0x7) << 12
                                    | (ppu->regbus & 0xf8) << 2);
            } else {
                // NOTE: set course and fine x
                ppu->t = (ppu->t & 0x7fe0) | (ppu->regbus & 0xf8) >> 3;
                ppu->x = ppu->regbus & 0x7;
            }
            ppu->w = !ppu->w;
        }
        break;
    case 6: // PPUADDR
        if (ppu->res != CSGS_SERVICED) {
            if (ppu->w) {
                ppu->t = (ppu->t & 0x7f00) | ppu->regbus;
                ppu->v = ppu->t;
            } else {
                ppu->t = (uint16_t)((ppu->t & 0xff)
                                    | (ppu->regbus & 0x3f) << 8);
            }
            ppu->w = !ppu->w;
            // TODO: there is some kind of bus conflict behavior i don't
            // understand yet, as well as palette corruption.
        }
        break;
    default:
        break;
    }
    return true;
}

//
// Internal Operations
//

static void nextdot(struct rp2c02 *self)
{
    if (++self->dot >= Dots) {
        self->dot = 0;
        if (++self->line >= Lines) {
            self->line = 0;
            self->odd = !self->odd;
        }
    }
}

static void reset(struct rp2c02 *self)
{
    // NOTE: t is cleared but NOT v
    self->dot = self->line = self->t = self->rbuf = self->x = 0;
    self->signal.intr = true;
    self->signal.vout = self->odd = self->w = false;
    set_ctrl(self, 0);
    set_mask(self, 0);
    self->res = CSGS_SERVICED;
}

// NOTE: this follows the same sequence as CPU reset sequence and is checked
// only on CPU-cycle boundaries rather than each PPU cycle; this keeps the two
// chips in sync when handling the RESET signal.
static void handle_reset(struct rp2c02 *self)
{
    // NOTE: pending always proceeds to committed just like the CPU sequence
    if (self->res == CSGS_PENDING) {
        self->res = CSGS_COMMITTED;
    }

    if (self->signal.res) {
        switch (self->res) {
        case CSGS_DETECTED:
            self->res = CSGS_CLEAR;
            break;
        case CSGS_COMMITTED:
            reset(self);
            break;
        default:
            break;
        }
    } else {
        switch (self->res) {
        case CSGS_CLEAR:
        case CSGS_SERVICED:
            self->res = CSGS_DETECTED;
            break;
        case CSGS_DETECTED:
            self->res = CSGS_PENDING;
            break;
        default:
            // NOTE: as long as reset line is held low there is no further
            // effect on PPU execution.
            break;
        }
    }
}

static int cycle(struct rp2c02 *self)
{
    // NOTE: vblank
    if (self->line == LineVBlank && self->dot == 0) {
        // NOTE: set vblank status 1 dot early to account for race-condition
        // that will suppress NMI if status.v is read (and cleared) right
        // before NMI is signaled on 241,1.
        // TODO: status.v should return false if read on 241,1 despite being "prepped" to true here
        self->status.v = true;
    } else if (in_vblank(self)) {
        // NOTE: NMI active within vblank if ctrl.v and status.v are set
        self->signal.intr = !self->ctrl.v || !self->status.v;
    } else if (self->line == LinePreRender && self->dot == 1) {
        // TODO: this is also pre-render line
        self->signal.intr = true;
        set_status(self, 0);
        self->res = CSGS_CLEAR;
        // TODO: add odd dot skip when rendering enabled
    }

    // NOTE: dot advancement happens last, leaving PPU on next dot to be drawn;
    // analogous to stack pointer always pointing at next byte to be written.
    nextdot(self);

    return 1;
}

//
// Public Interface
//

void ppu_connect(struct rp2c02 *self, bus *mbus)
{
    assert(self != NULL);
    assert(mbus != NULL);

    const bool r = bus_set(mbus, MEMBLOCK_8KB, (struct busdevice){
        .read = reg_read,
        .write = reg_write,
        .ctx = self,
    });
    assert(r);
}

void ppu_powerup(struct rp2c02 *self)
{
    assert(self != NULL);
    assert(self->vbus != NULL);

    // NOTE: NTSC PPU:CPU cycle ratio
    self->cyr = 3;

    // NOTE: initialize ppu to known state; internal components affected by the
    // reset sequence are deferred until that phase.
    self->regsel = self->oamaddr = 0;
    self->signal.intr = self->signal.res = self->signal.rw = self->signal.vr =
        self->signal.vw = true;
    self->signal.vout = self->status.s = false;

    // NOTE: simulate res set on startup to engage reset sequence
    self->res = CSGS_PENDING;
}

int ppu_cycle(struct rp2c02 *self, int cycles)
{
    assert(self != NULL);

    // NOTE: for simplicity, handle RESET signal on CPU cycle boundaries
    handle_reset(self);

    const int total = cycles * self->cyr;
    cycles = 0;
    for (int i = 0; i < total; ++i, cycles += cycle(self));
    return cycles;
}

void ppu_snapshot(const struct rp2c02 *self, struct snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);

    snp->ppu.ctrl = get_ctrl(self);
    snp->ppu.mask = get_mask(self);
    snp->ppu.oamaddr = self->oamaddr;
    snp->ppu.status = get_status(self);

    snp->pdatapath.res = self->res;
    snp->pdatapath.addressbus = self->vaddrbus;
    snp->pdatapath.curraddr = self->v;
    snp->pdatapath.tempaddr = self->t;
    snp->pdatapath.dot = self->dot;
    snp->pdatapath.line = self->line;
    snp->pdatapath.databus = self->vdatabus;
    snp->pdatapath.readbuffer = self->rbuf;
    snp->pdatapath.register_databus = self->regbus;
    snp->pdatapath.register_select = self->regsel;
    snp->pdatapath.xfine = self->x;
    snp->pdatapath.oddframe = self->odd;
    snp->pdatapath.writelatch = self->w;

    snp->plines.cpu_readwrite = self->signal.rw;
    snp->plines.interrupt = self->signal.intr;
    snp->plines.read = self->signal.vr;
    snp->plines.reset = self->signal.res;
    snp->plines.video_out = self->signal.vout;
    snp->plines.write = self->signal.vw;
}

struct ppu_coord ppu_trace(const struct rp2c02 *self, int adjustment)
{
    struct ppu_coord c = {
        self->dot + (adjustment * self->cyr),
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
