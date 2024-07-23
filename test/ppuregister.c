//
//  ppuregister.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 7/7/24.
//

#include "bus.h"
#include "ciny.h"
#include "ctrlsignal.h"
#include "ppu.h"
#include "ppuhelp.h"
#include "snapshot.h"

#include <stdint.h>

//
// MARK: - PPUCTRL
//

static void ppuctrl_write(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.ctrl);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2000, 0xff);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0xbfu, snp.ppu.ctrl);
    ct_assertequal(0xc00u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppuctrl_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->t = 0x7fff;
    ppu->ctrl.nh = ppu->ctrl.nl = true;

    ppu_snapshot(ppu, &snp);
    ct_assertequal(3u, snp.ppu.ctrl);
    ct_assertequal(0x7fffu, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3210, 0xfc);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0xbcu, snp.ppu.ctrl);
    ct_assertequal(0x73ffu, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xfcu, ppu->regbus);
}

static void ppuctrl_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->rst = CSGS_SERVICED;

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.ctrl);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2000, 0xff);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.ctrl);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppuctrl_read(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2000, &d);

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - PPUMASK
//

static void ppumask_write(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2001, 0xff);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0xffu, snp.ppu.mask);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3211, 0xff);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0xffu, snp.ppu.mask);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->rst = CSGS_SERVICED;

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2001, 0xff);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_read(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2001, &d);

    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - PPUSTATUS
//

static void ppustatus_read_when_clear(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->regbus = 0x5a;
    ppu->status.v = ppu->status.s = ppu->status.o = false;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x1au, d);
    ct_assertequal(0x1au, ppu->regbus);
    ct_assertequal(0u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_when_set(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->regbus = 0x5a;
    ppu->w = ppu->status.v = ppu->status.s = ppu->status.o = true;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xfau, d);
    ct_assertequal(0xfau, ppu->regbus);
    ct_assertequal(0x60u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_on_nmi_race_condition(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->regbus = 0x5a;
    ppu->status.v = ppu->status.s = ppu->status.o = true;
    ppu->line = 241;
    ppu->dot = 1;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x7au, d);
    ct_assertequal(0x7au, ppu->regbus);
    ct_assertequal(0x60u, snp.ppu.status);
}

static void ppustatus_read_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->regbus = 0x5a;
    ppu->w = ppu->status.v = ppu->status.s = ppu->status.o = true;
    ppu->rst = CSGS_SERVICED;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xfau, d);
    ct_assertequal(0xfau, ppu->regbus);
    ct_assertequal(0x60u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x3212, &d);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x1au, d);
    ct_assertequal(0x1au, ppu->regbus);
    ct_assertequal(0u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_write(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    struct snapshot snp;
    ppu->w = true;

    bus_write(ppt_get_mbus(ctx), 0x2002, 0xff);

    ppu_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
    ct_assertequal(0u, snp.ppu.status);
    ct_asserttrue(ppu->w);
}

//
// MARK: - OAMADDR
//

static void oamaddr_write(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2003, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3213, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->rst = CSGS_SERVICED;

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2003, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_read(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2003, &d);

    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - OAMDATA
//

static void oamdata_write(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_write_during_rendering(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 24;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oam[0] = ppu->oam[4] = ppu->oam[8] = ppu->oam[12] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->oam[0]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x8u, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->oam[4]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0xcu, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->oam[8]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x10u, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->oam[12]);
}

static void oamdata_write_during_rendering_disabled(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 24;
    ppu->dot = 5;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3214, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(ppt_get_mbus(ctx), 0x3214, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(ppt_get_mbus(ctx), 0x3214, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(ppt_get_mbus(ctx), 0x3214, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oam[0] = ppu->oam[1] = ppu->oam[2] = ppu->oam[3] = 0xff;
    ppu->rst = CSGS_SERVICED;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->oam[0]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->oam[1]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->oam[2]);

    bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->oam[3]);
}

static void oamdata_read(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oam[0] = 0x11;
    ppu->oam[1] = 0x22;
    ppu->oam[2] = 0x33;
    ppu->oam[3] = 0x44;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2004, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);
}

static void oamdata_read_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->ctrl.s = true;
    ppu->oam[0] = 0x11;
    ppu->oam[1] = 0x22;
    ppu->oam[2] = 0x33;
    ppu->oam[3] = 0x44;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x3214, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);
}

//
// MARK: - PPUSCROLL
//

static void ppuscroll_write(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);

    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2005, 0xaf); // 0b1010'1111

    ct_assertequal(0x15u, ppu->t);              // 0b000'0000'0001'0101
    ct_assertequal(7u, ppu->x);                 // 0b111
    ct_asserttrue(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xafu, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2005, 0xab); // 0b1010'1011

    ct_assertequal(0x32b5u, ppu->t);            // 0b011'0010'1011'0101
    ct_assertequal(7u, ppu->x);                 // 0b111
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuscroll_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->t = 0x7fff;
    ppu->x = 0xff;

    ct_assertequal(0x7fffu, ppu->t);
    ct_assertequal(0xffu, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3215, 0xaa); // 0b1010'1010

    ct_assertequal(0x7ff5u, ppu->t);            // 0b111'1111'1111'0101
    ct_assertequal(2u, ppu->x);                 // 0b010
    ct_asserttrue(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3215, 0xab); // 0b1010'1011

    ct_assertequal(0x3eb5u, ppu->t);            // 0b011'1110'1011'0101
    ct_assertequal(2u, ppu->x);                 // 0b010
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuscroll_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->rst = CSGS_SERVICED;

    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2005, 0xaf);

    ct_assertequal(0x0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xafu, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2005, 0xab);

    ct_assertequal(0x0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuscroll_read(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2005, &d);

    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - PPUADDR
//

static void ppuaddr_write(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4000;

    ct_assertequal(0x4000u, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2006, 0xa5); // 0b1010'0101

    ct_assertequal(0x2500u, ppu->t);            // 0b010'0101'0000'0000
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xa5u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x2006, 0xab); // 0b1010'1011

    ct_assertequal(0x25abu, ppu->t);            // 0b010'0101'1010'1011
    ct_assertequal(0x25abu, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuaddr_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4fff;

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3216, 0xa5); // 0b1010'0101

    ct_assertequal(0x25ffu, ppu->t);            // 0b010'0101'1111'1111
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xa5u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3216, 0xab); // 0b1010'1011

    ct_assertequal(0x25abu, ppu->t);            // 0b010'0101'1010'1011
    ct_assertequal(0x25abu, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuaddr_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4fff;
    ppu->rst = CSGS_SERVICED;

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3216, 0xa5);

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xa5u, ppu->regbus);

    bus_write(ppt_get_mbus(ctx), 0x3216, 0xab);

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuaddr_read(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2006, &d);

    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

// NOTE: recreates example from
// https://www.nesdev.org/wiki/PPU_scrolling#Details
static void ppu_addr_scroll_interleave(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4000;

    bus_write(ppt_get_mbus(ctx), 0x2006, 0x4);

    ct_assertequal(0u, ppu->x);
    ct_assertequal(0x400u, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);

    bus_write(ppt_get_mbus(ctx), 0x2005, 0x3e);

    ct_assertequal(0u, ppu->x);
    ct_assertequal(0x64e0u, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);

    bus_write(ppt_get_mbus(ctx), 0x2005, 0x7d);

    ct_assertequal(5u, ppu->x);
    ct_assertequal(0x64efu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);

    bus_write(ppt_get_mbus(ctx), 0x2006, 0xef);

    ct_assertequal(5u, ppu->x);
    ct_assertequal(0x64efu, ppu->t);
    ct_assertequal(0x64efu, ppu->v);
    ct_assertfalse(ppu->w);
}

//
// MARK: - PPUDATA
//

static void ppudata_write_in_vblank(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);
}

static void ppudata_write_with_row_increment(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = ppu->ctrl.i = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2022u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2022u, ppu->v);
}

static void ppudata_write_rendering_disabled(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 42;
    ppu->dot = 24;
    ppu->v = 0x2002;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);
}

static void ppudata_write_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;

    bus_write(ppt_get_mbus(ctx), 0x3217, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);
}

static void ppudata_write_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rst = CSGS_SERVICED;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);
}

static void ppudata_write_ignores_high_v_bits(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0xf002;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0xf002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0xf002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0xf003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0xf003u, ppu->v);
}

static void ppudata_write_palette_backdrop(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f00u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f00u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f01u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f01u, ppu->v);
}

static void ppudata_write_palette_background(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f06;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[6]);
    ct_assertequal(0x3f06u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[6]);
    ct_assertequal(0x3f06u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[6]);
    ct_assertequal(0x3f07u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[6]);
    ct_assertequal(0x3f07u, ppu->v);
}

static void ppudata_write_palette_last_background(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0f;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[15]);
    ct_assertequal(0x3f0fu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[15]);
    ct_assertequal(0x3f0fu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[15]);
    ct_assertequal(0x3f10u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[15]);
    ct_assertequal(0x3f10u, ppu->v);
}

static void ppudata_write_palette_unused(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0c;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f0cu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f0cu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f0du, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f0du, ppu->v);
}

static void ppudata_write_palette_backdrop_mirror(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f10;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f10u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f10u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f11u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f11u, ppu->v);
}

static void ppudata_write_palette_backdrop_high_mirror(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3ff0;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3ff0u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3ff0u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3ff1u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3ff1u, ppu->v);
}

static void ppudata_write_palette_sprite(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f16;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[20]);
    ct_assertequal(0x3f16u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[20]);
    ct_assertequal(0x3f16u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[20]);
    ct_assertequal(0x3f17u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[20]);
    ct_assertequal(0x3f17u, ppu->v);
}

static void ppudata_write_palette_last_sprite(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1f;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[27]);
    ct_assertequal(0x3f1fu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[27]);
    ct_assertequal(0x3f1fu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[27]);
    ct_assertequal(0x3f20u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[27]);
    ct_assertequal(0x3f20u, ppu->v);
}

static void ppudata_write_palette_unused_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1c;

    bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f1cu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f1cu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f1du, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[12]);
    ct_assertequal(0x3f1du, ppu->v);
}

static void ppudata_write_during_rendering(void *ctx)
{
    ct_assertfail("implement test");
}

static void ppudata_read_in_vblank(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);
    ct_assertequal(0xaau, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_with_row_increment(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = ppu->ctrl.i = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);
    ct_assertequal(0xaau, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2022u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2022u, ppu->v);

    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_rendering_disabled(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 42;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);
    ct_assertequal(0xaau, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x3217, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);
    ct_assertequal(0xaau, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    bus_read(ppt_get_mbus(ctx), 0x3217, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_during_reset(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;
    ppu->rst = CSGS_SERVICED;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);
    ct_assertequal(0xaau, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_ignores_high_v_bits(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0xf002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);
    ct_assertequal(0xaau, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0xf002u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0xf002u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0xf003u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0xf003u, ppu->v);

    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_palette_backdrop(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;
    ppu->palette[0] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f00u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f00u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f01u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f01u, ppu->v);
}

static void ppudata_read_palette_background(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f06;
    ppu->palette[6] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f06u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f06u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x3f07u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x3f07u, ppu->v);
}

static void ppudata_read_palette_last_background(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0f;
    ppu->palette[15] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0fu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0fu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x3f10u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x3f10u, ppu->v);
}

static void ppudata_read_palette_unused(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0c;
    ppu->palette[12] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0cu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0cu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f0du, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f0du, ppu->v);
}

static void ppudata_read_palette_backdrop_mirror(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f10;
    ppu->palette[0] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f10u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f10u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f11u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f11u, ppu->v);
}

static void ppudata_read_palette_backdrop_high_mirror(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3ff0;
    ppu->palette[0] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3ff0u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3ff0u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3ff1u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3ff1u, ppu->v);
}

static void ppudata_read_palette_sprite(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f16;
    ppu->palette[20] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f16u, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f16u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x3f17u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x3f17u, ppu->v);
}

static void ppudata_read_palette_last_sprite(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1f;
    ppu->palette[27] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1fu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1fu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x3f20u, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x3f20u, ppu->v);
}

static void ppudata_read_palette_unused_mirrored(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1c;
    ppu->palette[12] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1cu, ppu->v);

    ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1cu, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f1du, ppu->v);

    ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f1du, ppu->v);
}

static void ppudata_read_during_rendering(void *ctx)
{
    ct_assertfail("implement test");
}

static void ppudata_read_vram_behind_palette(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;
    ppu->palette[0] = 0xcc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);
    ppu_cycle(ppu);
    ppu_cycle(ppu);
    ppu_cycle(ppu);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xccu, ppu->regbus);
    ct_assertequal(0xccu, d);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f01u, ppu->v);

    bus_write(ppt_get_mbus(ctx), 0x2006, 0x20);
    bus_write(ppt_get_mbus(ctx), 0x2006, 0x2);
    bus_read(ppt_get_mbus(ctx), 0x2007, &d);
    ppu_cycle(ppu);
    ppu_cycle(ppu);
    ppu_cycle(ppu);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);
}

//
// MARK: - Test List
//

struct ct_testsuite ppu_register_tests(void)
{
    static const struct ct_testcase tests[] = {
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

        ct_maketest(ppuscroll_write),
        ct_maketest(ppuscroll_write_mirrored),
        ct_maketest(ppuscroll_write_during_reset),
        ct_maketest(ppuscroll_read),

        ct_maketest(ppuaddr_write),
        ct_maketest(ppuaddr_write_mirrored),
        ct_maketest(ppuaddr_write_during_reset),
        ct_maketest(ppuaddr_read),
        ct_maketest(ppu_addr_scroll_interleave),

        ct_maketest(ppudata_write_in_vblank),
        ct_maketest(ppudata_write_with_row_increment),
        ct_maketest(ppudata_write_rendering_disabled),
        ct_maketest(ppudata_write_mirrored),
        ct_maketest(ppudata_write_during_reset),
        ct_maketest(ppudata_write_ignores_high_v_bits),
        ct_maketest(ppudata_write_palette_backdrop),
        ct_maketest(ppudata_write_palette_background),
        ct_maketest(ppudata_write_palette_last_background),
        ct_maketest(ppudata_write_palette_unused),
        ct_maketest(ppudata_write_palette_backdrop_mirror),
        ct_maketest(ppudata_write_palette_backdrop_high_mirror),
        ct_maketest(ppudata_write_palette_sprite),
        ct_maketest(ppudata_write_palette_last_sprite),
        ct_maketest(ppudata_write_palette_unused_mirrored),
        ct_maketest(ppudata_write_during_rendering),
        ct_maketest(ppudata_read_in_vblank),
        ct_maketest(ppudata_read_with_row_increment),
        ct_maketest(ppudata_read_rendering_disabled),
        ct_maketest(ppudata_read_mirrored),
        ct_maketest(ppudata_read_during_reset),
        ct_maketest(ppudata_read_ignores_high_v_bits),
        ct_maketest(ppudata_read_palette_backdrop),
        ct_maketest(ppudata_read_palette_background),
        ct_maketest(ppudata_read_palette_last_background),
        ct_maketest(ppudata_read_palette_unused),
        ct_maketest(ppudata_read_palette_backdrop_mirror),
        ct_maketest(ppudata_read_palette_backdrop_high_mirror),
        ct_maketest(ppudata_read_palette_sprite),
        ct_maketest(ppudata_read_palette_last_sprite),
        ct_maketest(ppudata_read_palette_unused_mirrored),
        ct_maketest(ppudata_read_during_rendering),
        ct_maketest(ppudata_read_vram_behind_palette),
    };

    return ct_makesuite_setup_teardown(tests, ppu_setup, ppu_teardown);
}
