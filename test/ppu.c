//
//  ppu.c
//  Tests
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "bus.h"
#include "bytes.h"
#include "ciny.h"
#include "ctrlsignal.h"
#include "ppu.h"

#include <stdint.h>
#include <stdlib.h>

#define get_ppu(ctx) &((struct test_context *)(ctx))->ppu
#define get_mbus(ctx) ((struct test_context *)(ctx))->mbus

// TODO: not used yet
static uint8_t VRam[256] = {0};

struct test_context {
    struct rp2c02 ppu;
    bus *mbus, *vbus;
};

static void setup(void **ctx)
{
    struct test_context *const c = malloc(sizeof *c);
    // NOTE: enough main bus to map $2000 - $3FFF for ppu registers
    c->mbus = bus_new(BITWIDTH_16KB, 2, MEMBLOCK_8KB);
    c->ppu.vbus = c->vbus = bus_new(BITWIDTH_16KB, 1);
    ppu_connect(&c->ppu, c->mbus);
    // NOTE: run powerup and reset sequence and then force internal state to a
    // known zero-value.
    ppu_powerup(&c->ppu);
    c->ppu.cyr = 1;
    ppu_cycle(&c->ppu, 1);
    c->ppu.line = c->ppu.dot = 0;
    c->ppu.res = CSGS_CLEAR;
    *ctx = c;
}

static void teardown(void **ctx)
{
    struct test_context *const c = (struct test_context *)*ctx;
    bus_free(c->vbus);
    bus_free(c->mbus);
    free(c);
}

static void powerup_initializes_ppu(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);

    ppu_powerup(ppu);

    ct_assertequal(3u, ppu->cyr);
    ct_assertequal(CSGS_PENDING, (int)ppu->res);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->oamaddr);
    ct_asserttrue(ppu->signal.intr);
    ct_asserttrue(ppu->signal.res);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->signal.vr);
    ct_asserttrue(ppu->signal.vw);
    ct_assertfalse(ppu->signal.vout);
    ct_assertfalse(ppu->status.s);
}

static void ppuctrl_write(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);

    ct_assertfalse(ppu->ctrl.nl);
    ct_assertfalse(ppu->ctrl.nh);
    ct_assertfalse(ppu->ctrl.i);
    ct_assertfalse(ppu->ctrl.s);
    ct_assertfalse(ppu->ctrl.b);
    ct_assertfalse(ppu->ctrl.h);
    ct_assertfalse(ppu->ctrl.p);
    ct_assertfalse(ppu->ctrl.v);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2000, 0xff);

    ct_asserttrue(ppu->ctrl.nl);
    ct_asserttrue(ppu->ctrl.nh);
    ct_asserttrue(ppu->ctrl.i);
    ct_asserttrue(ppu->ctrl.s);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->ctrl.h);
    ct_assertfalse(ppu->ctrl.p);
    ct_asserttrue(ppu->ctrl.v);
    ct_assertequal(0xc00u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppuctrl_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);

    ct_assertfalse(ppu->ctrl.nl);
    ct_assertfalse(ppu->ctrl.nh);
    ct_assertfalse(ppu->ctrl.i);
    ct_assertfalse(ppu->ctrl.s);
    ct_assertfalse(ppu->ctrl.b);
    ct_assertfalse(ppu->ctrl.h);
    ct_assertfalse(ppu->ctrl.p);
    ct_assertfalse(ppu->ctrl.v);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x3210, 0xff);

    ct_asserttrue(ppu->ctrl.nl);
    ct_asserttrue(ppu->ctrl.nh);
    ct_asserttrue(ppu->ctrl.i);
    ct_asserttrue(ppu->ctrl.s);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->ctrl.h);
    ct_assertfalse(ppu->ctrl.p);
    ct_asserttrue(ppu->ctrl.v);
    ct_assertequal(0xc00u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppuctrl_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->res = CSGS_SERVICED;

    ct_assertfalse(ppu->ctrl.nl);
    ct_assertfalse(ppu->ctrl.nh);
    ct_assertfalse(ppu->ctrl.i);
    ct_assertfalse(ppu->ctrl.s);
    ct_assertfalse(ppu->ctrl.b);
    ct_assertfalse(ppu->ctrl.h);
    ct_assertfalse(ppu->ctrl.p);
    ct_assertfalse(ppu->ctrl.v);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2000, 0xff);

    ct_assertfalse(ppu->ctrl.nl);
    ct_assertfalse(ppu->ctrl.nh);
    ct_assertfalse(ppu->ctrl.i);
    ct_assertfalse(ppu->ctrl.s);
    ct_assertfalse(ppu->ctrl.b);
    ct_assertfalse(ppu->ctrl.h);
    ct_assertfalse(ppu->ctrl.p);
    ct_assertfalse(ppu->ctrl.v);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppuctrl_read(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2000, &d);

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

static void ppumask_write(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);

    ct_assertfalse(ppu->mask.g);
    ct_assertfalse(ppu->mask.bm);
    ct_assertfalse(ppu->mask.sm);
    ct_assertfalse(ppu->mask.b);
    ct_assertfalse(ppu->mask.s);
    ct_assertfalse(ppu->mask.re);
    ct_assertfalse(ppu->mask.ge);
    ct_assertfalse(ppu->mask.be);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2001, 0xff);

    ct_asserttrue(ppu->mask.g);
    ct_asserttrue(ppu->mask.bm);
    ct_asserttrue(ppu->mask.sm);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->mask.s);
    ct_asserttrue(ppu->mask.re);
    ct_asserttrue(ppu->mask.ge);
    ct_asserttrue(ppu->mask.be);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);

    ct_assertfalse(ppu->mask.g);
    ct_assertfalse(ppu->mask.bm);
    ct_assertfalse(ppu->mask.sm);
    ct_assertfalse(ppu->mask.b);
    ct_assertfalse(ppu->mask.s);
    ct_assertfalse(ppu->mask.re);
    ct_assertfalse(ppu->mask.ge);
    ct_assertfalse(ppu->mask.be);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x3211, 0xff);

    ct_asserttrue(ppu->mask.g);
    ct_asserttrue(ppu->mask.bm);
    ct_asserttrue(ppu->mask.sm);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->mask.s);
    ct_asserttrue(ppu->mask.re);
    ct_asserttrue(ppu->mask.ge);
    ct_asserttrue(ppu->mask.be);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->res = CSGS_SERVICED;

    ct_assertfalse(ppu->mask.g);
    ct_assertfalse(ppu->mask.bm);
    ct_assertfalse(ppu->mask.sm);
    ct_assertfalse(ppu->mask.b);
    ct_assertfalse(ppu->mask.s);
    ct_assertfalse(ppu->mask.re);
    ct_assertfalse(ppu->mask.ge);
    ct_assertfalse(ppu->mask.be);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2001, 0xff);

    ct_assertfalse(ppu->mask.g);
    ct_assertfalse(ppu->mask.bm);
    ct_assertfalse(ppu->mask.sm);
    ct_assertfalse(ppu->mask.b);
    ct_assertfalse(ppu->mask.s);
    ct_assertfalse(ppu->mask.re);
    ct_assertfalse(ppu->mask.ge);
    ct_assertfalse(ppu->mask.be);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_read(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2001, &d);

    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

static void ppustatus_read_when_clear(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;
    ppu->status.v = ppu->status.s = ppu->status.o = false;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2002, &d);

    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x1au, d);
    ct_assertequal(0x1au, ppu->regbus);
    ct_assertfalse(ppu->status.o);
    ct_assertfalse(ppu->status.s);
    ct_assertfalse(ppu->status.v);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_when_set(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;
    ppu->w = ppu->status.v = ppu->status.s = ppu->status.o = true;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2002, &d);

    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xfau, d);
    ct_assertequal(0xfau, ppu->regbus);
    ct_asserttrue(ppu->status.o);
    ct_asserttrue(ppu->status.s);
    ct_assertfalse(ppu->status.v);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_on_nmi_race_condition(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;
    ppu->status.v = ppu->status.s = ppu->status.o = true;
    ppu->line = 241;
    ppu->dot = 1;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2002, &d);

    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x7au, d);
    ct_assertequal(0x7au, ppu->regbus);
    ct_asserttrue(ppu->status.o);
    ct_asserttrue(ppu->status.s);
    ct_assertfalse(ppu->status.v);
}

static void ppustatus_read_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;
    ppu->w = ppu->status.v = ppu->status.s = ppu->status.o = true;
    ppu->res = CSGS_SERVICED;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2002, &d);

    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xfau, d);
    ct_assertequal(0xfau, ppu->regbus);
    ct_asserttrue(ppu->status.o);
    ct_asserttrue(ppu->status.s);
    ct_assertfalse(ppu->status.v);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;
    ppu->status.v = ppu->status.s = ppu->status.o = false;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x3212, &d);

    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x1au, d);
    ct_assertequal(0x1au, ppu->regbus);
    ct_assertfalse(ppu->status.o);
    ct_assertfalse(ppu->status.s);
    ct_assertfalse(ppu->status.v);
    ct_assertfalse(ppu->w);
}

static void ppustatus_write(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->w = true;
    ppu->status.v = ppu->status.s = ppu->status.o = false;

    bus_write(get_mbus(ctx), 0x2002, 0xff);

    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
    ct_assertfalse(ppu->status.o);
    ct_assertfalse(ppu->status.s);
    ct_assertfalse(ppu->status.v);
    ct_asserttrue(ppu->w);
}

static void oamaddr_write(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2003, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x3213, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->res = CSGS_SERVICED;

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2003, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_read(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2003, &d);

    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

static void oamdata_write(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oamaddr = ppu->oamdata = 0;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oamdata);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oamdata);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oamdata);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oamdata);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_write_during_rendering(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 24;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oamaddr = ppu->oamdata = 0;
    ppu->oam[0] = ppu->oam[4] = ppu->oam[8] = ppu->oam[12] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oamdata);
    ct_assertequal(0xffu, ppu->oam[0]);

    bus_write(get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x8u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oamdata);
    ct_assertequal(0xffu, ppu->oam[4]);

    bus_write(get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0xcu, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oamdata);
    ct_assertequal(0xffu, ppu->oam[8]);

    bus_write(get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x10u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oamdata);
    ct_assertequal(0xffu, ppu->oam[12]);
}

static void oamdata_write_during_rendering_disabled(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 24;
    ppu->dot = 5;
    ppu->oamaddr = ppu->oamdata = 0;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oamdata);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oamdata);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oamdata);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oamdata);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oamaddr = ppu->oamdata = 0;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x3214, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oamdata);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(get_mbus(ctx), 0x3214, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oamdata);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(get_mbus(ctx), 0x3214, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oamdata);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(get_mbus(ctx), 0x3214, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oamdata);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oamaddr = ppu->oamdata = 0;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;
    ppu->res = CSGS_SERVICED;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oamdata);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oamdata);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oamdata);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oamdata);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_read(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oamaddr = ppu->oamdata = 0;
    ppu->oam[0] = 0x11;
    ppu->oam[1] = 0x22;
    ppu->oam[2] = 0x33;
    ppu->oam[3] = 0x44;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x2004, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x11u, ppu->oamdata);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);
}

static void oamdata_read_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oamaddr = ppu->oamdata = 0;
    ppu->oam[0] = 0x11;
    ppu->oam[1] = 0x22;
    ppu->oam[2] = 0x33;
    ppu->oam[3] = 0x44;

    uint8_t d;
    bus_read(get_mbus(ctx), 0x3214, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x11u, ppu->oamdata);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);
}

static void reset_sequence(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    // NOTE: reset cadence is on cpu cycle so ppu cycle ratio doesn't matter
    ppu->cyr = 3;
    ppu->line = 42;
    ppu->dot = 24;
    ppu->signal.intr = false;
    ppu->ctrl.b = ppu->mask.b = ppu->signal.vout = ppu->odd = ppu->w = true;
    ppu->scroll = 0x45;
    ppu->addr = 0x9a;
    ppu->rbuf = 0x56;
    ppu->x = 0x78;
    ppu->t = ppu->v = 0x1234;

    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(27u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_CLEAR, (int)ppu->res);

    ppu->signal.res = false;
    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(30u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_DETECTED, (int)ppu->res);

    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(33u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_PENDING, (int)ppu->res);

    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(36u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_COMMITTED, (int)ppu->res);

    // NOTE: reset line held
    ppu_cycle(ppu, 5);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(51u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_COMMITTED, (int)ppu->res);

    ppu->signal.res = true;
    ppu_cycle(ppu, 1);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(3u, ppu->dot);
    ct_asserttrue(ppu->signal.intr);
    ct_assertfalse(ppu->ctrl.b);
    ct_assertfalse(ppu->mask.b);
    ct_assertfalse(ppu->signal.vout);
    ct_assertfalse(ppu->odd);
    ct_assertfalse(ppu->w);
    ct_assertequal(0x0u, ppu->scroll);
    ct_assertequal(0x0u, ppu->addr);
    ct_assertequal(0x0u, ppu->rbuf);
    ct_assertequal(0x0u, ppu->x);
    ct_assertequal(0x0u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_SERVICED, (int)ppu->res);
}

static void reset_too_short(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    // NOTE: reset cadence is on cpu cycle so ppu cycle ratio doesn't matter
    ppu->cyr = 3;
    ppu->line = 42;
    ppu->dot = 24;
    ppu->signal.intr = false;
    ppu->ctrl.b = ppu->mask.b = ppu->signal.vout = ppu->odd = ppu->w = true;
    ppu->scroll = 0x45;
    ppu->addr = 0x9a;
    ppu->rbuf = 0x56;
    ppu->x = 0x78;
    ppu->t = ppu->v = 0x1234;

    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(27u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_CLEAR, (int)ppu->res);

    ppu->signal.res = false;
    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(30u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_DETECTED, (int)ppu->res);

    ppu->signal.res = true;
    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(33u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x45u, ppu->scroll);
    ct_assertequal(0x9au, ppu->addr);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_CLEAR, (int)ppu->res);
}

static void vblank_prep(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->status.v = false;
    ppu->ctrl.v = true;
    ppu->line = 241;

    ppu_cycle(ppu, 1);

    ct_assertequal(1u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_start(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->status.v = true;
    ppu->ctrl.v = true;
    ppu->line = 241;
    ppu->dot = 1;

    ppu_cycle(ppu, 1);

    ct_assertequal(2u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);
}

static void vblank_start_nmi_disabled(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->status.v = true;
    ppu->line = 241;
    ppu->dot = 1;

    ppu_cycle(ppu, 1);

    ct_assertequal(2u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_start_nmi_missed(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->status.v = false;
    ppu->ctrl.v = true;
    ppu->line = 241;
    ppu->dot = 1;

    ppu_cycle(ppu, 1);

    ct_assertequal(2u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_nmi_toggle(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->status.v = true;
    ppu->ctrl.v = true;
    ppu->line = 250;
    ppu->dot = 40;
    ppu->signal.intr = false;

    ppu_cycle(ppu, 1);

    ct_assertequal(41u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);

    ppu->ctrl.v = false;
    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);

    ppu->ctrl.v = true;
    ppu_cycle(ppu, 1);

    ct_assertequal(43u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);
}

static void vblank_nmi_clear(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->status.v = true;
    ppu->ctrl.v = true;
    ppu->line = 250;
    ppu->dot = 40;
    ppu->signal.intr = false;

    ppu_cycle(ppu, 1);

    ct_assertequal(41u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);

    ppu->status.v = false;
    ppu_cycle(ppu, 1);

    ct_assertequal(42u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);

    ppu_cycle(ppu, 1);

    ct_assertequal(43u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_end(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->status.v = true;
    ppu->status.s = true;
    ppu->status.o = true;
    ppu->ctrl.v = true;
    ppu->line = 261;
    ppu->dot = 1;
    ppu->res = CSGS_SERVICED;
    ppu->signal.intr = false;

    ppu_cycle(ppu, 1);

    ct_assertequal(2u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_assertfalse(ppu->status.s);
    ct_assertfalse(ppu->status.o);
    ct_asserttrue(ppu->signal.intr);
    ct_assertequal(CSGS_CLEAR, (int)ppu->res);
}

static void frame_toggle(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->line = 261;
    ppu->dot = 340;

    ct_assertfalse(ppu->odd);

    ppu_cycle(ppu, 1);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(0u, ppu->dot);
    ct_asserttrue(ppu->odd);

    ppu->line = 261;
    ppu->dot = 340;
    ppu_cycle(ppu, 1);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(0u, ppu->dot);
    ct_assertfalse(ppu->odd);
}

static void trace_no_adjustment(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->cyr = 3;

    const struct ppu_coord pixel = ppu_trace(ppu, 0);

    ct_assertequal(0, pixel.dot);
    ct_assertequal(0, pixel.line);
}

static void trace_zero_with_adjustment(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->cyr = 3;

    const struct ppu_coord pixel = ppu_trace(ppu, -1);

    ct_assertequal(338, pixel.dot);
    ct_assertequal(261, pixel.line);
}

static void trace(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->cyr = 3;
    ppu->line = 120;
    ppu->dot = 223;

    const struct ppu_coord pixel = ppu_trace(ppu, -1);

    ct_assertequal(220, pixel.dot);
    ct_assertequal(120, pixel.line);
}

static void trace_at_one_cycle(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->cyr = 3;
    ppu->dot = 3;

    const struct ppu_coord pixel = ppu_trace(ppu, -1);

    ct_assertequal(0, pixel.dot);
    ct_assertequal(0, pixel.line);
}

static void trace_at_line_boundary(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->cyr = 3;
    ppu->line = 120;
    ppu->dot = 1;

    const struct ppu_coord pixel = ppu_trace(ppu, -1);

    ct_assertequal(339, pixel.dot);
    ct_assertequal(119, pixel.line);
}

//
// Test List
//

struct ct_testsuite ppu_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_ppu),

        ct_maketest(ppuctrl_write),
        ct_maketest(ppuctrl_write_mirrored),
        ct_maketest(ppuctrl_write_during_reset),
        ct_maketest(ppuctrl_read),
        ct_maketest(ppumask_write),
        ct_maketest(ppumask_write_mirrored),
        ct_maketest(ppumask_write_during_reset),
        ct_maketest(ppumask_read),
        ct_maketest(ppustatus_read_when_clear),
        ct_maketest(ppustatus_read_when_set),
        ct_maketest(ppustatus_read_on_nmi_race_condition),
        ct_maketest(ppustatus_read_during_reset),
        ct_maketest(ppustatus_read_mirrored),
        ct_maketest(ppustatus_write),
        ct_maketest(oamaddr_write),
        ct_maketest(oamaddr_write_mirrored),
        ct_maketest(oamaddr_write_during_reset),
        ct_maketest(oamaddr_read),
        ct_maketest(oamdata_write),
        ct_maketest(oamdata_write_during_rendering),
        ct_maketest(oamdata_write_during_rendering_disabled),
        ct_maketest(oamdata_write_mirrored),
        ct_maketest(oamdata_write_during_reset),
        ct_maketest(oamdata_read),
        ct_maketest(oamdata_read_mirrored),

        ct_maketest(reset_sequence),
        ct_maketest(reset_too_short),

        ct_maketest(vblank_prep),
        ct_maketest(vblank_start),
        ct_maketest(vblank_start_nmi_disabled),
        ct_maketest(vblank_start_nmi_missed),
        ct_maketest(vblank_nmi_toggle),
        ct_maketest(vblank_nmi_clear),
        ct_maketest(vblank_end),
        ct_maketest(frame_toggle),

        ct_maketest(trace_no_adjustment),
        ct_maketest(trace_zero_with_adjustment),
        ct_maketest(trace),
        ct_maketest(trace_at_one_cycle),
        ct_maketest(trace_at_line_boundary),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
