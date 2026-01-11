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
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.ctrl);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2000, 0xff);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0xbfu, snp.ppu.ctrl);
    ct_assertequal(0xc00u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppuctrl_write_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->t = 0x7fff;
    ppu->ctrl.nh = ppu->ctrl.nl = true;

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(3u, snp.ppu.ctrl);
    ct_assertequal(0x7fffu, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3210, 0xfc);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0xbcu, snp.ppu.ctrl);
    ct_assertequal(0x73ffu, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xfcu, ppu->regbus);
}

static void ppuctrl_write_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->rst = ALDO_SIG_SERVICED;

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.ctrl);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2000, 0xff);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.ctrl);
    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppuctrl_read(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2000, &d);

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - PPUMASK
//

static void ppumask_write(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2001, 0xff);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0xffu, snp.ppu.mask);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_write_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3211, 0xff);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0xffu, snp.ppu.mask);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_write_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->rst = ALDO_SIG_SERVICED;

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2001, 0xff);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(0u, snp.ppu.mask);
    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0xffu, ppu->regbus);
}

static void ppumask_read(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2001, &d);

    ct_assertequal(1u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - PPUSTATUS
//

static void ppustatus_read_when_clear(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->regbus = 0x5a;
    ppu->status.v = ppu->status.s = ppu->status.o = false;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x1au, d);
    ct_assertequal(0x1au, ppu->regbus);
    ct_assertequal(0u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_when_set(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->regbus = 0x5a;
    ppu->w = ppu->status.v = ppu->status.s = ppu->status.o = true;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xfau, d);
    ct_assertequal(0xfau, ppu->regbus);
    ct_assertequal(0x60u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_on_nmi_race_condition(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->regbus = 0x5a;
    ppu->status.v = ppu->status.s = ppu->status.o = true;
    ppu->line = 241;
    ppu->dot = 1;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x7au, d);
    ct_assertequal(0x7au, ppu->regbus);
    ct_assertequal(0x60u, snp.ppu.status);
}

static void ppustatus_read_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->regbus = 0x5a;
    ppu->w = ppu->status.v = ppu->status.s = ppu->status.o = true;
    ppu->rst = ALDO_SIG_SERVICED;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2002, &d);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0xfau, d);
    ct_assertequal(0xfau, ppu->regbus);
    ct_assertequal(0x60u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_read_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->regbus = 0x5a;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x3212, &d);

    aldo_ppu_bus_snapshot(ppu, &snp);
    ct_assertequal(2u, ppu->regsel);
    ct_assertequal(0x1au, d);
    ct_assertequal(0x1au, ppu->regbus);
    ct_assertequal(0u, snp.ppu.status);
    ct_assertfalse(ppu->w);
}

static void ppustatus_write(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    struct aldo_snapshot snp;
    ppu->w = true;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2002, 0xff);

    aldo_ppu_bus_snapshot(ppu, &snp);
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
    auto ppu = ppt_get_ppu(ctx);

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2003, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_write_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3213, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_write_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->rst = ALDO_SIG_SERVICED;

    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2003, 0xc3);

    ct_assertequal(0xc3u, ppu->oamaddr);
    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0xc3u, ppu->regbus);
}

static void oamaddr_read(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2003, &d);

    ct_assertequal(3u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - OAMDATA
//

static void oamdata_write(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->mask.s = true;
    auto spr = &ppu->spr;
    spr->oam[0] = spr->oam[1] = spr->oam[2] = spr->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->spr.oam[0]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->spr.oam[1]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->spr.oam[2]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->spr.oam[3]);
}

static void oamdata_write_during_rendering(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 24;
    ppu->dot = 5;
    ppu->mask.s = true;
    ppu->spr.oam[0] = ppu->spr.oam[4] = ppu->spr.oam[8] = ppu->spr.oam[12] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->spr.oam[0]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x8u, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->spr.oam[4]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0xcu, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->spr.oam[8]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x10u, ppu->oamaddr);
    ct_assertequal(0xffu, ppu->spr.oam[12]);
}

static void oamdata_write_during_rendering_disabled(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 24;
    ppu->dot = 5;
    auto spr = &ppu->spr;
    spr->oam[0] = spr->oam[1] = spr->oam[2] = spr->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->spr.oam[0]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->spr.oam[1]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->spr.oam[2]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->spr.oam[3]);
}

static void oamdata_write_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->mask.s = true;
    auto spr = &ppu->spr;
    spr->oam[0] = spr->oam[1] = spr->oam[2] = spr->oam[3] = 0xff;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3214, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->spr.oam[0]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3214, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->spr.oam[1]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3214, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->spr.oam[2]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3214, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->spr.oam[3]);
}

static void oamdata_write_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->mask.s = true;
    auto spr = &ppu->spr;
    spr->oam[0] = spr->oam[1] = spr->oam[2] = spr->oam[3] = 0xff;
    ppu->rst = ALDO_SIG_SERVICED;

    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x11);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0x1u, ppu->oamaddr);
    ct_assertequal(0x11u, ppu->spr.oam[0]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x22);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x22u, ppu->regbus);
    ct_assertequal(0x2u, ppu->oamaddr);
    ct_assertequal(0x22u, ppu->spr.oam[1]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x33);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x3u, ppu->oamaddr);
    ct_assertequal(0x33u, ppu->spr.oam[2]);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2004, 0x44);

    ct_assertequal(0x4u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x4u, ppu->oamaddr);
    ct_assertequal(0x44u, ppu->spr.oam[3]);
}

static void oamdata_read(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->mask.s = true;
    ppu->spr.oam[0] = 0x11;
    ppu->spr.oam[1] = 0x22;
    ppu->spr.oam[2] = 0x33;
    ppu->spr.oam[3] = 0x44;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2004, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);
}

static void oamdata_read_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 240;
    ppu->dot = 5;
    ppu->mask.s = true;
    ppu->spr.oam[0] = 0x11;
    ppu->spr.oam[1] = 0x22;
    ppu->spr.oam[2] = 0x33;
    ppu->spr.oam[3] = 0x44;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x3214, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);
}

static void oamdata_read_during_soam_clear(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 0;
    ppu->mask.s = true;
    ppu->spr.oam[0] = 0x11;
    ppu->spr.oam[1] = 0x22;
    ppu->spr.oam[2] = 0x33;
    ppu->spr.oam[3] = 0x44;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2004, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0x11u, d);
    ct_assertequal(0x11u, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);

    ppu->dot = 1;

    aldo_bus_read(ppt_get_mbus(ctx), 0x2004, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0xffu, d);
    ct_assertequal(0xffu, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);

    ppu->dot = 64;

    aldo_bus_read(ppt_get_mbus(ctx), 0x2004, &d);

    ct_assertequal(4u, ppu->regsel);
    ct_assertequal(0xffu, d);
    ct_assertequal(0xffu, ppu->regbus);
    ct_assertequal(0u, ppu->oamaddr);

    ppu->dot = 65;

    aldo_bus_read(ppt_get_mbus(ctx), 0x2004, &d);

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
    auto ppu = ppt_get_ppu(ctx);

    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2005, 0xaf); // 0b1010'1111

    ct_assertequal(0x15u, ppu->t);              // 0b000'0000'0001'0101
    ct_assertequal(7u, ppu->x);                 // 0b111
    ct_asserttrue(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xafu, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2005, 0xab); // 0b1010'1011

    ct_assertequal(0x32b5u, ppu->t);            // 0b011'0010'1011'0101
    ct_assertequal(7u, ppu->x);                 // 0b111
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuscroll_write_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->t = 0x7fff;
    ppu->x = 0xff;

    ct_assertequal(0x7fffu, ppu->t);
    ct_assertequal(0xffu, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3215, 0xaa); // 0b1010'1010

    ct_assertequal(0x7ff5u, ppu->t);            // 0b111'1111'1111'0101
    ct_assertequal(2u, ppu->x);                 // 0b010
    ct_asserttrue(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xaau, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3215, 0xab); // 0b1010'1011

    ct_assertequal(0x3eb5u, ppu->t);            // 0b011'1110'1011'0101
    ct_assertequal(2u, ppu->x);                 // 0b010
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuscroll_write_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->rst = ALDO_SIG_SERVICED;

    ct_assertequal(0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2005, 0xaf);

    ct_assertequal(0x0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xafu, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2005, 0xab);

    ct_assertequal(0x0u, ppu->t);
    ct_assertequal(0u, ppu->x);
    ct_assertfalse(ppu->w);
    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuscroll_read(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2005, &d);

    ct_assertequal(5u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

//
// MARK: - PPUADDR
//

static void ppuaddr_write(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4000;

    ct_assertequal(0x4000u, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2006, 0xa5); // 0b1010'0101

    ct_assertequal(0x2500u, ppu->t);            // 0b010'0101'0000'0000
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xa5u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2006, 0xab); // 0b1010'1011

    ct_assertequal(0x25abu, ppu->t);            // 0b010'0101'1010'1011
    ct_assertequal(0x25abu, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuaddr_write_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4fff;

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3216, 0xa5); // 0b1010'0101

    ct_assertequal(0x25ffu, ppu->t);            // 0b010'0101'1111'1111
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xa5u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3216, 0xab); // 0b1010'1011

    ct_assertequal(0x25abu, ppu->t);            // 0b010'0101'1010'1011
    ct_assertequal(0x25abu, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuaddr_write_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4fff;
    ppu->rst = ALDO_SIG_SERVICED;

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3216, 0xa5);

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xa5u, ppu->regbus);

    aldo_bus_write(ppt_get_mbus(ctx), 0x3216, 0xab);

    ct_assertequal(0x4fffu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);
    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0xabu, ppu->regbus);
}

static void ppuaddr_read(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->regbus = 0x5a;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2006, &d);

    ct_assertequal(6u, ppu->regsel);
    ct_assertequal(0x5au, d);
}

// NOTE: recreates example from
// https://www.nesdev.org/wiki/PPU_scrolling#Details
static void ppu_addr_scroll_interleave(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->t = 0x4000;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2006, 0x4);

    ct_assertequal(0u, ppu->x);
    ct_assertequal(0x400u, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2005, 0x3e);

    ct_assertequal(0u, ppu->x);
    ct_assertequal(0x64e0u, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_assertfalse(ppu->w);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2005, 0x7d);

    ct_assertequal(5u, ppu->x);
    ct_assertequal(0x64efu, ppu->t);
    ct_assertequal(0u, ppu->v);
    ct_asserttrue(ppu->w);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2006, 0xef);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = ppu->ctrl.i = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2022u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 42;
    ppu->dot = 24;
    ppu->v = 0x2002;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;

    aldo_bus_write(ppt_get_mbus(ctx), 0x3217, 0x77);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rst = ALDO_SIG_SERVICED;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0xf002;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x33u, VRam[2]);
    ct_assertequal(0xf002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[2]);
    ct_assertequal(0xf003u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f00u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f00u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f01u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f01u, ppu->v);
}

static void ppudata_write_palette_background(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f06;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[6]);
    ct_assertequal(0x3f06u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[6]);
    ct_assertequal(0x3f06u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[6]);
    ct_assertequal(0x3f07u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[6]);
    ct_assertequal(0x3f07u, ppu->v);
}

static void ppudata_write_palette_last_background(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0f;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[15]);
    ct_assertequal(0x3f0fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[15]);
    ct_assertequal(0x3f0fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[15]);
    ct_assertequal(0x3f10u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[15]);
    ct_assertequal(0x3f10u, ppu->v);
}

static void ppudata_write_palette_unused(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0c;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f0cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f0cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f0du, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f0du, ppu->v);
}

static void ppudata_write_palette_backdrop_mirror(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f10;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f10u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f10u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f11u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3f11u, ppu->v);
}

static void ppudata_write_palette_backdrop_high_mirror(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3ff0;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3ff0u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3ff0u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3ff1u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[0]);
    ct_assertequal(0x3ff1u, ppu->v);
}

static void ppudata_write_palette_high_bits(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.g = ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f00u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f01u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x77u, ppu->palette[0]);
    ct_assertequal(0x3f01u, ppu->v);
}

static void ppudata_write_palette_sprite(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f16;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[20]);
    ct_assertequal(0x3f16u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[20]);
    ct_assertequal(0x3f16u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[20]);
    ct_assertequal(0x3f17u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[20]);
    ct_assertequal(0x3f17u, ppu->v);
}

static void ppudata_write_palette_last_sprite(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1f;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[27]);
    ct_assertequal(0x3f1fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[27]);
    ct_assertequal(0x3f1fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[27]);
    ct_assertequal(0x3f20u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[27]);
    ct_assertequal(0x3f20u, ppu->v);
}

static void ppudata_write_palette_unused_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1c;

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x37);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x37u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f1cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f1cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f1du, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0x37u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x11u, VRam[0]);
    ct_assertequal(0x37u, ppu->palette[12]);
    ct_assertequal(0x3f1du, ppu->v);
}

static void ppudata_write_during_rendering(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 9;       // ppudata ALE cycle starts on tile ALE
    ppu->v = 0x43a3;    // 100 00 11101 00011

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x44u, VRam[3]);
    ct_assertequal(0x43a3u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x23a3u, ppu->vaddrbus); // Tile NT address put on bus
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x44u, VRam[3]);
    ct_assertequal(0x43a3u, ppu->v);
    ct_assertequal(10, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23a3u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[3]);
    // PPUDATA write during rendering triggers course-x and y increment
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23f9u, ppu->vaddrbus);  // Tile AT address put on bus
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[3]);
    ct_assertequal(0x53a4u, ppu->v);
}

static void ppudata_write_during_rendering_off_sync(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 10;      // ppudata ALE cycle starts on tile read
    ppu->v = 0x43a3;    // 100 00 11101 00011

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x22u, VRam[1]);
    ct_assertequal(0x43a3u, ppu->v);

    // simulate the pipeline putting something on the addrbus
    ppu->vaddrbus = 0x3235;
    aldo_ppu_cycle(ppu);

    // ALE latch cycle is skipped, write wherever addrbus is pointing
    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3235u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[1]);
    // PPUDATA write during rendering triggers course-x and y increment
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(11, ppu->dot);
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23f9u, ppu->vaddrbus);  // Tile AT address put on bus
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[1]);
    ct_assertequal(0x53a4u, ppu->v);
}

static void ppudata_write_during_rendering_on_x_increment(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 15;      // ppudata ALE cycle starts on tile BGH ALE
    ppu->v = 0x43a3;    // 100 00 11101 00011

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x43a3u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus); // Tile BGH address put on bus
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x43a3u, ppu->v);
    ct_assertequal(16, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_asserttrue(ppu->bflt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    // PPUDATA write during rendering doesn't increment x twice
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23a4u, ppu->vaddrbus);  // Tile NT address put on bus
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x53a4u, ppu->v);
}

static void ppudata_write_during_rendering_on_row_increment(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 255;     // ppudata ALE cycle starts on tile BGH ALE
    ppu->v = 0x43a3;    // 100 00 11101 00011

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x43a3u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus); // Tile BGH address put on bus
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x43a3u, ppu->v);
    ct_assertequal(256, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_asserttrue(ppu->bflt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    // PPUDATA write during rendering doesn't increment x and y twice
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23a4u, ppu->vaddrbus);  // Tile NT address put on bus
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x53a0u, ppu->v);    // t(x) copied to v
}

static void ppudata_write_during_sprite_rendering(void *ctx)
{
    ct_ignore("not implemented");

    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 270;     // ppudata ALE cycle starts on sprite FGL read
    ppu->v = 0x43a3;    // 100 00 11101 00011

    aldo_bus_write(ppt_get_mbus(ctx), 0x2007, 0x77);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x77u, ppu->regbus);
    ct_assertfalse(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x22u, VRam[1]);
    ct_assertequal(0x43a3u, ppu->v);

    // simulate the pipeline putting something on the addrbus
    ppu->vaddrbus = 0x3235;
    aldo_ppu_cycle(ppu);

    // ALE latch cycle is skipped, write wherever addrbus is pointing
    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3235u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[1]);
    // PPUDATA write during rendering triggers course-x and y increment
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(11, ppu->dot);
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23f9u, ppu->vaddrbus);  // Tile AT address put on bus
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.wr);
    ct_assertequal(0x77u, VRam[1]);
    ct_assertequal(0x53a4u, ppu->v);
}

static void ppudata_read_in_vblank(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_with_row_increment(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = ppu->ctrl.i = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2022u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2022u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_rendering_disabled(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 42;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_mirrored(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x3217, &d);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x3217, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_during_reset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x2002;
    ppu->rbuf = 0xaa;
    ppu->rst = ALDO_SIG_SERVICED;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x2002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x2002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x2003u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_ignores_high_v_bits(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0xf002;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0xf002u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0xf003u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3002u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0xf003u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x33u, ppu->regbus);
    ct_assertequal(0x33u, d);
}

static void ppudata_read_palette_backdrop(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;
    ppu->palette[0] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f00u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f00u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f01u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f06;
    ppu->palette[6] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f06u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f06u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f06u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x3f07u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0f;
    ppu->palette[15] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0fu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x3f10u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f0c;
    ppu->palette[12] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f0cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f0cu, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f0du, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f10;
    ppu->palette[0] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f10u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f10u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f10u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f11u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3ff0;
    ppu->palette[0] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3ff0u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3ff0u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3ff1u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3ff0u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3ff1u, ppu->v);
}

static void ppudata_read_palette_with_high_bits(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.g = ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;
    ppu->palette[0] = 0xfc;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x30u, ppu->regbus);
    ct_assertequal(0x30u, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f00u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f00u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f01u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f01u, ppu->v);
}

static void ppudata_read_palette_sprite(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f16;
    ppu->palette[20] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f16u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f16u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f16u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x33u, ppu->rbuf);
    ct_assertequal(0x3f17u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1f;
    ppu->palette[27] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1fu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1fu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x3f20u, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f1c;
    ppu->palette[12] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x3f1cu, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x3f1cu, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f1du, ppu->v);

    aldo_ppu_cycle(ppu);

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
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 9;       // ppudata ALE cycle starts on tile ALE
    ppu->v = 0x43a3;    // 100 00 11101 00011
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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
    ct_assertequal(0x43a3u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x23a3u, ppu->vaddrbus); // Tile NT address put on bus
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x43a3u, ppu->v);
    ct_assertequal(10, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23a3u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    // PPUDATA read during rendering triggers course-x and y increment
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23f9u, ppu->vaddrbus);  // Tile AT address put on bus
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x53a4u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x44u, d);
}

static void ppudata_read_during_rendering_off_sync(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 10;      // ppudata ALE cycle starts on tile read
    ppu->v = 0x43a3;    // 100 00 11101 00011
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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
    ct_assertequal(0x43a3u, ppu->v);

    // simulate the pipeline putting something on the databus
    ppu->vdatabus = 0xee;
    aldo_ppu_cycle(ppu);

    // ALE latch cycle is skipped, whatever's on the databus is read to rbuf
    ct_assertfalse(ppu->cvp);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_asserttrue(ppu->bflt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0xeeu, ppu->rbuf);
    // PPUDATA read during rendering triggers course-x and y increment
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(11, ppu->dot);
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23f9u, ppu->vaddrbus);  // Tile AT address put on bus
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xeeu, ppu->rbuf);
    ct_assertequal(0x53a4u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xeeu, ppu->regbus);
    ct_assertequal(0xeeu, d);
}

static void ppudata_read_during_rendering_on_x_increment(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 15;      // ppudata ALE cycle starts on tile BGH ALE
    ppu->v = 0x43a3;    // 100 00 11101 00011
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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
    ct_assertequal(0x43a3u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus); // Tile BGH address put on bus
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x43a3u, ppu->v);
    ct_assertequal(16, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.atb);

    // simulate the pipeline putting something on the databus
    ppu->vdatabus = 0xee;
    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_asserttrue(ppu->bflt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0xeeu, ppu->rbuf);
    // PPUDATA read during rendering doesn't increment x twice
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23a4u, ppu->vaddrbus);  // Tile NT address put on bus
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xeeu, ppu->rbuf);
    ct_assertequal(0x53a4u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xeeu, ppu->regbus);
    ct_assertequal(0xeeu, d);
}

static void ppudata_read_during_rendering_on_row_increment(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 255;     // ppudata ALE cycle starts on tile BGH ALE
    ppu->v = 0x43a3;    // 100 00 11101 00011
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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
    ct_assertequal(0x43a3u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus); // Tile BGH address put on bus
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x43a3u, ppu->v);
    ct_assertequal(256, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.atb);

    // simulate the pipeline putting something on the databus
    ppu->vdatabus = 0xee;
    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0xcu, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_asserttrue(ppu->bflt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0xeeu, ppu->rbuf);
    // PPUDATA read during rendering doesn't increment x and y twice
    ct_assertequal(0x53a4u, ppu->v);    // 101 00 11101 00100
    ct_assertequal(0x2u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x23a4u, ppu->vaddrbus);  // Tile NT address put on bus
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xeeu, ppu->rbuf);
    ct_assertequal(0x53a0u, ppu->v);    // t(x) copied to v

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0xeeu, ppu->regbus);
    ct_assertequal(0xeeu, d);
}

static void ppudata_read_during_sprite_rendering(void *ctx)
{
    ct_ignore("not implemented");

    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 42;
    ppu->dot = 270;     // ppudata ALE cycle starts on sprite FGL read
    ppu->v = 0x43a3;    // 100 00 11101 00011
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

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
    ct_assertequal(0x13a3u, ppu->v);

    aldo_ppu_cycle(ppu);

    ct_asserttrue(ppu->cvp);
    ct_assertequal(0x13a3u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0xaau, ppu->rbuf);
    ct_assertequal(0x13a3u, ppu->v);
    ct_assertequal(271, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x13a3u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    // PPUDATA read during rendering triggers course-x and y increment
    ct_assertequal(0x23a4u, ppu->v);    // 010 00 11101 00100
    ct_assertequal(0x1u, ppu->pxpl.atb);

    aldo_ppu_cycle(ppu);

    ct_assertfalse(ppu->cvp);
    ct_assertequal(0x13a3u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertequal(0x44u, ppu->rbuf);
    ct_assertequal(0x23a4u, ppu->v);

    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x44u, ppu->regbus);
    ct_assertequal(0x44u, d);
}

static void ppudata_read_vram_behind_palette(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->mask.b = ppu->mask.s = true;
    ppu->line = 242;
    ppu->dot = 24;
    ppu->v = 0x3f00;
    ppu->palette[0] = 0x2c;
    ppu->rbuf = 0xaa;

    uint8_t d;
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);
    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

    ct_assertequal(7u, ppu->regsel);
    ct_assertequal(0x2cu, ppu->regbus);
    ct_assertequal(0x2cu, d);
    ct_assertequal(0x3f00u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->rbuf);
    ct_assertequal(0x3f01u, ppu->v);

    aldo_bus_write(ppt_get_mbus(ctx), 0x2006, 0x20);
    aldo_bus_write(ppt_get_mbus(ctx), 0x2006, 0x2);
    aldo_bus_read(ppt_get_mbus(ctx), 0x2007, &d);
    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

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

struct ct_testsuite ppu_register_tests()
{
    static constexpr struct ct_testcase tests[] = {
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
        ct_maketest(oamdata_read_during_soam_clear),

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
        ct_maketest(ppudata_write_palette_high_bits),
        ct_maketest(ppudata_write_palette_sprite),
        ct_maketest(ppudata_write_palette_last_sprite),
        ct_maketest(ppudata_write_palette_unused_mirrored),
        ct_maketest(ppudata_write_during_rendering),
        ct_maketest(ppudata_write_during_rendering_off_sync),
        ct_maketest(ppudata_write_during_rendering_on_x_increment),
        ct_maketest(ppudata_write_during_rendering_on_row_increment),
        ct_maketest(ppudata_write_during_sprite_rendering),
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
        ct_maketest(ppudata_read_palette_with_high_bits),
        ct_maketest(ppudata_read_palette_sprite),
        ct_maketest(ppudata_read_palette_last_sprite),
        ct_maketest(ppudata_read_palette_unused_mirrored),
        ct_maketest(ppudata_read_during_rendering),
        ct_maketest(ppudata_read_during_rendering_off_sync),
        ct_maketest(ppudata_read_during_rendering_on_x_increment),
        ct_maketest(ppudata_read_during_rendering_on_row_increment),
        ct_maketest(ppudata_read_during_sprite_rendering),
        ct_maketest(ppudata_read_vram_behind_palette),
    };

    return ct_makesuite_setup_teardown(tests, ppu_setup, ppu_teardown);
}
