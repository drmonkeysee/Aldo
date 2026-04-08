//
//  ppurender.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 10/22/24.
//

#include "bus.h"
#include "bytes.h"
#include "ciny.h"
#include "ppuhelp.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define memfill(mem) memset(mem, 0xff, aldo_arrsz(mem))

static uint8_t
        NameTables[2][8],
        AttributeTables[2][8],
        PatternTables[2][16];

static bool chrread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    static constexpr auto ptsz = aldo_arrsz(PatternTables[0]);

    if (addr < ALDO_MEMBLOCK_8KB) {
        auto ptable = addr < 0x1000 ? PatternTables[0] : PatternTables[1];
        *d = ptable[addr % ptsz];
        return true;
    }
    return false;
}

static bool vramread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    static constexpr auto ntsz = aldo_arrsz(NameTables[0]);
    static constexpr auto atsz = aldo_arrsz(AttributeTables[0]);

    if (ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB) {
        // use horizontal mirroring for testing (Donkey Kong setting)
        auto select = addr < 0x2800 ? 0 : 1;
        *d = (addr & 0x3ff) < 0x3c0
                ? NameTables[select][addr % ntsz]
                : AttributeTables[select][addr % atsz];
        return true;
    }
    return false;
}

static void init_palette(struct aldo_rp2c02 *ppu)
{
    // BG 1
    ppu->palette[0] = 0x24; // Magenta (almost) as backdrop
    ppu->palette[1] = 0x0;
    ppu->palette[2] = 0x1;
    ppu->palette[3] = 0x2;

    // BG 2
    ppu->palette[4] = 0xd;  // Unused slot, this should always output backdrop
    ppu->palette[5] = 0x10;
    ppu->palette[6] = 0x11;
    ppu->palette[7] = 0x12;

    // BG 3
    ppu->palette[8] = 0xd;  // Unused slot, this should always output backdrop
    ppu->palette[9] = 0x5;
    ppu->palette[10] = 0x6;
    ppu->palette[11] = 0x7;

    // BG 4
    ppu->palette[12] = 0xd;  // Unused slot, this should always output backdrop
    ppu->palette[13] = 0x15;
    ppu->palette[14] = 0x16;
    ppu->palette[15] = 0x17;

    // FG 1
    ppu->palette[16] = 0x20;
    ppu->palette[17] = 0x21;
    ppu->palette[18] = 0x22;

    // FG 2
    ppu->palette[19] = 0x29;
    ppu->palette[20] = 0x2a;
    ppu->palette[21] = 0x2b;

    // FG 3
    ppu->palette[22] = 0x33;
    ppu->palette[23] = 0x34;
    ppu->palette[24] = 0x35;

    // FG 4
    ppu->palette[25] = 0x38;
    ppu->palette[26] = 0x39;
    ppu->palette[27] = 0x3a;
}

static void ppu_render_setup(void **ctx)
{
    void *v;
    ppu_setup(&v);
    struct ppu_test_context *c = v;
    aldo_bus_set(c->vbus, 0, (struct aldo_busdevice){.read = chrread});
    aldo_bus_set(c->vbus, ALDO_MEMBLOCK_8KB, (struct aldo_busdevice){
        .read = vramread,
    });
    init_palette(&c->ppu);
    // turn on rendering and turn left-column mask off
    c->ppu.mask.b = c->ppu.mask.s = c->ppu.mask.bm = c->ppu.mask.sm = true;
    *ctx = v;
}

static void ppu_render_teardown(void **ctx)
{
    aldo_memclr(NameTables[0]);
    aldo_memclr(NameTables[1]);
    aldo_memclr(AttributeTables[0]);
    aldo_memclr(AttributeTables[1]);
    aldo_memclr(PatternTables[0]);
    aldo_memclr(PatternTables[1]);

    ppu_teardown(ctx);
}

//
// MARK: - Basic Tile Fetches
//

static void nametable_fetch(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 1;  // avoid odd-frame skipped-dot behavior on line 0

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void attributetable_fetch(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    AttributeTables[0][1] = 0x22;
    ppu->v = 0x5;
    ppu->dot = 3;

    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.tlu.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void tile_fetch(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->dot = 5;
    ppu->pxpl.tlu.nt = 0x11;
    ppu->pxpl.tlu.atb = 4;

    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(4u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(4u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(4u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(6u, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void tile_fetch_higher_bits_sequence(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[1][2] = 0x11;
    AttributeTables[1][2] = 0x22;
    PatternTables[1][3] = 0x33;
    PatternTables[1][11] = 0x44;
    ppu->v = 0x3aea;
    ppu->ctrl.b = true;
    ppu->line = 1;  // avoid odd-frame skipped-dot behavior on line 0

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x1003u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.tlu.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x3aebu, ppu->v);
    ct_assertequal(6u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void tile_fetch_post_render_line(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[1][2] = 0x11;
    AttributeTables[1][2] = 0x22;
    PatternTables[1][3] = 0x33;
    PatternTables[1][11] = 0x44;
    ppu->v = 0x3aea;
    ppu->ctrl.b = true;
    ppu->line = 240;

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void tile_fetch_rendering_disabled(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[1][2] = 0x11;
    AttributeTables[1][2] = 0x22;
    PatternTables[1][3] = 0x33;
    PatternTables[1][11] = 0x44;
    ppu->v = 0x3aea;
    ppu->ctrl.b = true;
    ppu->line = 1;  // avoid odd-frame skipped-dot behavior on line 0
    ppu->mask.b = ppu->mask.s = false;

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void tile_fetch_tile_only_disabled(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[1][2] = 0x11;
    AttributeTables[1][2] = 0x22;
    PatternTables[1][3] = 0x33;
    PatternTables[1][11] = 0x44;
    ppu->v = 0x3aea;
    ppu->ctrl.b = true;
    ppu->line = 1;  // avoid odd-frame skipped-dot behavior on line 0
    ppu->mask.b = false;

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x1003u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.tlu.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x3aebu, ppu->v);
    ct_assertequal(6u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

//
// MARK: - Line Edge Cases
//

static void render_line_end(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][6] = 0x77;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 255;
    ppu->pxpl.tlu.nt = 0x11;
    ppu->pxpl.tlu.atb = 4;

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(256, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(4u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x1006u, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0x77u, ppu->pxpl.tlu.nt);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0x77u, ppu->pxpl.tlu.nt);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0x77u, ppu->pxpl.tlu.nt);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void render_line_prefetch(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    NameTables[0][6] = 0xaa;
    NameTables[0][7] = 0xee;
    AttributeTables[0][1] = 0x22;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->dot = 321;
    ppu->pxpl.tlu.atb = 4;

    // Tile Fetch 1
    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(322, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(323, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(324, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.at);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(325, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.tlu.at);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(326, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(327, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(328, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0u, ppu->pxpl.tlu.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(329, ppu->dot);
    ct_assertequal(6u, ppu->v);
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch 2
    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(330, ppu->dot);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(331, ppu->dot);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0xaau, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(332, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.tlu.at);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(333, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.tlu.at);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    // simulate getting a different tile
    PatternTables[0][0] = 0xbb;
    PatternTables[0][8] = 0xcc;

    aldo_ppu_cycle(ppu);

    ct_assertequal(334, ppu->dot);
    ct_assertequal(6u, ppu->v);
    ct_assertequal(0x0aa0u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(335, ppu->dot);
    ct_assertequal(6u, ppu->v);
    ct_assertequal(0x0aa0u, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(336, ppu->dot);
    ct_assertequal(6u, ppu->v);
    ct_assertequal(0x0aa8u, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.tlu.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(337, ppu->dot);
    ct_assertequal(7u, ppu->v);
    ct_assertequal(2u, ppu->pxpl.tlu.atb);
    ct_assertequal(0x0aa8u, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.tlu.bg[0]);
    ct_assertequal(0xccu, ppu->pxpl.tlu.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Garbage NT Fetches
    aldo_ppu_cycle(ppu);

    ct_assertequal(338, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0xaau, ppu->pxpl.tlu.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(339, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xeeu, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(340, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xeeu, ppu->pxpl.tlu.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xeeu, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->line);
    ct_assertequal(1, ppu->dot);
    ct_assertequal(7u, ppu->v);
    ct_assertequal(0x0ee0u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.tlu.bg[0]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void prerender_nametable_fetch(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 261;

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void prerender_end_even_frame(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 261;
    ppu->dot = 339;

    aldo_ppu_cycle(ppu);

    ct_assertequal(340, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertfalse(ppu->odd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0, ppu->line);
    ct_asserttrue(ppu->odd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void prerender_end_odd_frame(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 261;
    ppu->dot = 339;
    ppu->odd = true;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.tlu.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertfalse(ppu->odd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.tlu.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0, ppu->line);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void prerender_set_vertical_coords(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->v = 0x5;
    ppu->t = 0x5fba;
    ppu->dot = 279;
    ppu->line = 261;

    aldo_ppu_cycle(ppu);

    ct_assertequal(280, ppu->dot);
    ct_assertequal(5u, ppu->v);

    do {
        aldo_ppu_cycle(ppu);

        ct_assertequal(0x5ba5u, ppu->v);
        ppu->v = 0x5;
    } while (ppu->dot < 305);

    aldo_ppu_cycle(ppu);

    ct_assertequal(306, ppu->dot);
    ct_assertequal(5u, ppu->v);
}

static void render_not_set_vertical_coords(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->v = 0x5;
    ppu->t = 0x5fba;
    ppu->dot = 279;

    aldo_ppu_cycle(ppu);

    ct_assertequal(280, ppu->dot);
    ct_assertequal(5u, ppu->v);

    do {
        aldo_ppu_cycle(ppu);

        ct_assertequal(5u, ppu->v);
        ppu->v = 0x5;
    } while (ppu->dot < 305);

    aldo_ppu_cycle(ppu);

    ct_assertequal(306, ppu->dot);
    ct_assertequal(5u, ppu->v);
}

//
// MARK: - Sprite Fetches
//

static void sprite_fetch_first_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x44u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_last_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][7] = 0xaa;
    PatternTables[0][15] = 0xbb;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->line = 7;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x37u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x37u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x3fu, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x3fu, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0xbbu, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_vertical_flip(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][7] = 0xaa;
    PatternTables[0][15] = 0xbb;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x82;    // fv:1, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x37u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x37u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x3fu, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x3fu, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0xbbu, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_horizontal_flip(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x42;    // fv:0, hv:1, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x42u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x42u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x42u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x42u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x42u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0x42u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x22u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_vertical_and_horizontal_flip(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][7] = 0xaa;
    PatternTables[0][15] = 0xbb;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0xc2;    // fv:1, hv:1, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0xc2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0xc2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x37u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0xc2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x37u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0xc2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x55u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x3fu, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0xc2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x55u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x3fu, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0xc2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x55u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0xddu, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_upper_bank(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[1][0] = 0x66;
    PatternTables[1][8] = 0x88;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.s = true;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1030u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1030u, ppu->vaddrbus);
    ct_assertequal(0x66u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x66u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1038u, ppu->vaddrbus);
    ct_assertequal(0x66u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x66u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1038u, ppu->vaddrbus);
    ct_assertequal(0x88u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x66u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x88u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_top_first_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x40u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x40u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x48u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x48u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x44u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_top_last_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][7] = 0xaa;
    PatternTables[0][15] = 0xbb;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->line = 7;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x47u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x47u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x4fu, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x4fu, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0xbbu, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_bottom_first_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x11;
    PatternTables[0][8] = 0x22;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->line = 8;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x50u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x50u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x11u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x58u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x11u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x58u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x11u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x22u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_bottom_last_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][7] = 0xcc;
    PatternTables[0][15] = 0xdd;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->line = 15;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x57u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x57u, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x5fu, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x5fu, ppu->vaddrbus);
    ct_assertequal(0xddu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0xddu, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_vertical_flip_top_first_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][7] = 0xcc;
    PatternTables[0][15] = 0xdd;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x82;    // fv:1, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x57u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x57u, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x5fu, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x5fu, ppu->vaddrbus);
    ct_assertequal(0xddu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xccu, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0xddu, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_vertical_flip_top_last_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x11;
    PatternTables[0][8] = 0x22;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->line = 7;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x82;    // fv:1, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x50u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x50u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x11u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x58u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x11u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x58u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x11u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x22u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_vertical_flip_bottom_first_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][7] = 0xaa;
    PatternTables[0][15] = 0xbb;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->line = 8;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x82;    // fv:1, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x47u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x47u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x4fu, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x4fu, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0xaau, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0xbbu, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_vertical_flip_bottom_last_row(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->line = 15;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x4;     // tile id, lower bank
    ppu->spr.soam[2] = 0x82;    // fv:1, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x40u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x40u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x48u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x48u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0x82u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x44u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_dh_fetch_upper_bank(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[1][0] = 0x33;
    PatternTables[1][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x5;     // tile id, upper bank
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->ctrl.h = true;         // 8x16 sprites

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1040u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1040u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1048u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x1048u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x44u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_out_of_range_sprite(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 10;      // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_rendering_disabled(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->mask.b = ppu->mask.s = false;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(5u, ppu->v);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0xabu, ppu->oamaddr);
    ct_assertequal(72u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void sprite_fetch_sprite_only_disabled(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][5] = 0x77;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->spr.soam[0] = 0;       // y-coord
    ppu->spr.soam[1] = 0x3;     // tile id
    ppu->spr.soam[2] = 0x2;     // fv:0, hv:0, pri:fg, pal:2
    ppu->spr.soam[3] = 43;      // x-coord
    ppu->oamaddr = 0xab;
    ppu->mask.s = false;

    // Unused NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(0u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Ignored NT Fetch + Latch Sprite Attribute
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(0u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Latch Sprite X-Coord
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch Low
    aldo_ppu_cycle(ppu);

    ct_assertequal(262, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(263, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x30u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Sprite Fetch High
    aldo_ppu_cycle(ppu);

    ct_assertequal(264, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0u, ppu->pxpl.spu[0].fg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(265, ppu->dot);
    ct_assertequal(0x413u, ppu->v);
    ct_assertequal(0x38u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(1u, ppu->pxpl.spuidx);
    ct_assertequal(2u, ppu->pxpl.spu[0].a);
    ct_assertequal(43u, ppu->pxpl.spu[0].x);
    ct_assertequal(0x33u, ppu->pxpl.spu[0].fg[0]);
    ct_assertequal(0x44u, ppu->pxpl.spu[0].fg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void sprite_fetch_full_line_sequence(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->oamaddr = 0xab;
    for (size_t i = 0; i < aldo_arrsz(ppu->spr.soam); i += 4) {
        ppu->spr.soam[i] = 0;                       // y-coord
        ppu->spr.soam[i + 1] = (uint8_t)(i + 4);    // arbitrary tile id
        ppu->spr.soam[i + 2] = 0x2;                 // fv:0, hv:0, pri:fg, pal:2
        ppu->spr.soam[i + 3] = (uint8_t)(i + 23);   // arbitrary x-coord
    }

    // run entire sprite fetch sequence
    auto tile = 0;
    for (int i = 0; i < 64; ++i) {
        aldo_ppu_cycle(ppu);
        if (ppu->dot % 8 == 6) {
            // check set low pattern addr which is -1 from current ppu->dot value
            // set unique test patterns for each sprite unit
            PatternTables[0][0] = (uint8_t)i;
            PatternTables[0][8] = (uint8_t)(0x80 | i);
            tile += 4;  // capture i as tile ID for pattern asserts
            // assert low pattern addr
            auto expected = (uint16_t)(tile << 4);
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected low fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        } else if (ppu->dot % 8 == 0) {
            // check set high pattern addr which is -1 from current ppu->dot value
            // assert high pattern addr
            auto expected = (uint16_t)((tile << 4) | (1 << 3));
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected high fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        }
    }

    ct_assertequal(321, ppu->dot);
    ct_assertequal(8u, ppu->pxpl.spuidx);

    for (size_t i = 0, chr = 4, x = 0;
         i < aldo_arrsz(ppu->pxpl.spu);
         ++i, chr += 8, x += 4) {
        auto spu = ppu->pxpl.spu + i;
        ct_assertequal(0x2u, spu->a, "unexpected attribute value for spu %d", i);
        ct_assertequal(chr, spu->fg[0], "unexpected lo plane for spu %d", i);
        ct_assertequal(0x80 | chr, spu->fg[1], "unexpected hi plane for spu %d", i);
        ct_assertequal(x + 23, spu->x, "unexpected x-coord for spu %d", i);
    }
}

static void sprite_fetch_empty_line_sequence(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->oamaddr = 0xab;
    for (size_t i = 0; i < aldo_arrsz(ppu->spr.soam); i += 4) {
        ppu->spr.soam[i] = 10;                      // y-coord
        ppu->spr.soam[i + 1] = (uint8_t)(i + 4);    // arbitrary tile id
        ppu->spr.soam[i + 2] = 0x2;                 // fv:0, hv:0, pri:fg, pal:2
        ppu->spr.soam[i + 3] = (uint8_t)(i + 23);   // arbitrary x-coord
    }

    // run entire sprite fetch sequence
    auto tile = 0;
    for (int i = 0; i < 64; ++i) {
        aldo_ppu_cycle(ppu);
        if (ppu->dot % 8 == 6) {
            // check set low pattern addr which is -1 from current ppu->dot value
            // set unique test patterns for each sprite unit
            PatternTables[0][0] = (uint8_t)i;
            PatternTables[0][8] = (uint8_t)(0x80 | i);
            tile += 4;  // capture i as tile ID for pattern asserts
            // assert low pattern addr
            auto expected = (uint16_t)(tile << 4);
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected low fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        } else if (ppu->dot % 8 == 0) {
            // check set high pattern addr which is -1 from current ppu->dot value
            // assert high pattern addr
            auto expected = (uint16_t)((tile << 4) | (1 << 3));
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected high fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        }
    }

    ct_assertequal(321, ppu->dot);
    ct_assertequal(8u, ppu->pxpl.spuidx);

    for (size_t i = 0; i < aldo_arrsz(ppu->pxpl.spu); ++i) {
        auto spu = ppu->pxpl.spu + i;
        ct_assertequal(0u, spu->a, "unexpected attribute value for spu %d", i);
        ct_assertequal(0u, spu->fg[0], "unexpected lo plane for spu %d", i);
        ct_assertequal(0u, spu->fg[1], "unexpected hi plane for spu %d", i);
        ct_assertequal(0u, spu->x, "unexpected x-coord for spu %d", i);
    }
}

static void sprite_fetch_partial_line_sequence(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 257;
    ppu->pxpl.spuidx = 72;
    ppu->oamaddr = 0xab;
    for (size_t i = 0; i < aldo_arrsz(ppu->spr.soam); i += 4) {
        // set 5 sprites within range
        ppu->spr.soam[i] = i < 20 ? 0 : 10;         // y-coord
        ppu->spr.soam[i + 1] = (uint8_t)(i + 4);    // arbitrary tile id
        ppu->spr.soam[i + 2] = 0x2;                 // fv:0, hv:0, pri:fg, pal:2
        ppu->spr.soam[i + 3] = (uint8_t)(i + 23);   // arbitrary x-coord
    }

    // run entire sprite fetch sequence
    auto tile = 0;
    for (int i = 0; i < 64; ++i) {
        aldo_ppu_cycle(ppu);
        if (ppu->dot % 8 == 6) {
            // check set low pattern addr which is -1 from current ppu->dot value
            // set unique test patterns for each sprite unit
            PatternTables[0][0] = (uint8_t)i;
            PatternTables[0][8] = (uint8_t)(0x80 | i);
            tile += 4;  // capture i as tile ID for pattern asserts
            // assert low pattern addr
            auto expected = (uint16_t)(tile << 4);
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected low fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        } else if (ppu->dot % 8 == 0) {
            // check set high pattern addr which is -1 from current ppu->dot value
            // assert high pattern addr
            auto expected = (uint16_t)((tile << 4) | (1 << 3));
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected high fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        }
    }

    ct_assertequal(321, ppu->dot);
    ct_assertequal(8u, ppu->pxpl.spuidx);

    for (size_t i = 0, chr = 4, x = 0;
         i < aldo_arrsz(ppu->pxpl.spu);
         ++i, chr += 8, x += 4) {
        auto spu = ppu->pxpl.spu + i;
        uint8_t attr = 0, plane0 = 0, plane1 = 0, xcoord = 0;
        if (i < 5) {
            attr = 0x2;
            plane0 = (uint8_t)chr;
            plane1 = (uint8_t)(0x80 | chr);
            xcoord = (uint8_t)(x + 23);
        }
        ct_assertequal(attr, spu->a, "unexpected attribute value for spu %d", i);
        ct_assertequal(plane0, spu->fg[0], "unexpected lo plane for spu %d", i);
        ct_assertequal(plane1, spu->fg[1], "unexpected hi plane for spu %d", i);
        ct_assertequal(xcoord, spu->x, "unexpected x-coord for spu %d", i);
    }
}

static void sprite_fetch_starts_late(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    // simulate rendering enabled on this dot, 5 sprites in and spuidx was never reset
    ppu->dot = 297;
    ppu->pxpl.spuidx = 8;
    ppu->oamaddr = 0xab;
    for (size_t i = 0; i < aldo_arrsz(ppu->spr.soam); i += 4) {
        ppu->spr.soam[i] = 0;                       // y-coord
        ppu->spr.soam[i + 1] = (uint8_t)(i + 4);    // arbitrary tile id
        ppu->spr.soam[i + 2] = 0x2;                 // fv:0, hv:0, pri:fg, pal:2
        ppu->spr.soam[i + 3] = (uint8_t)(i + 23);   // arbitrary x-coord
    }

    // run rest of sprite fetch sequence
    auto tile = 0;
    for (int i = 0; i < 24; ++i) {
        aldo_ppu_cycle(ppu);
        if (ppu->dot % 8 == 6) {
            // check set low pattern addr which is -1 from current ppu->dot value
            // set unique test patterns for each sprite unit
            PatternTables[0][0] = (uint8_t)i;
            PatternTables[0][8] = (uint8_t)(0x80 | i);
            tile += 4;  // capture i as tile ID for pattern asserts
            // assert low pattern addr
            auto expected = (uint16_t)(tile << 4);
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected low fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        } else if (ppu->dot % 8 == 0) {
            // check set high pattern addr which is -1 from current ppu->dot value
            // assert high pattern addr
            auto expected = (uint16_t)((tile << 4) | (1 << 3));
            ct_assertequal(expected, ppu->vaddrbus,
                           "unexpected high fetch addr for sprite unit %d (%d)",
                           i / 8, i);
        }
    }

    ct_assertequal(321, ppu->dot);
    ct_assertequal(3u, ppu->pxpl.spuidx);

    for (size_t i = 0, chr = 4, x = 0;
         i < aldo_arrsz(ppu->pxpl.spu);
         ++i, chr += 8, x += 4) {
        auto spu = ppu->pxpl.spu + i;
        uint8_t attr = 0, plane0 = 0, plane1 = 0, xcoord = 0;
        if (i < 3) {
            attr = 0x2;
            plane0 = (uint8_t)chr;
            plane1 = (uint8_t)(0x80 | chr);
            xcoord = (uint8_t)(x + 23);
        }
        ct_assertequal(attr, spu->a, "unexpected attribute value for spu %d", i);
        ct_assertequal(plane0, spu->fg[0], "unexpected lo plane for spu %d", i);
        ct_assertequal(plane1, spu->fg[1], "unexpected hi plane for spu %d", i);
        ct_assertequal(xcoord, spu->x, "unexpected x-coord for spu %d", i);
    }
}

//
// MARK: - Coordinate Behaviors
//

static void course_x_wraparound(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->v = 0x1f;                  // 000 00 00000 11111
    ppu->dot = 8;

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x400u, ppu->v); // 000 01 00000 00000
    // atb set to previous course-x selection
    ct_assertequal(2u, ppu->pxpl.tlu.atb);

    ppu->v = 0x41f;                 // 000 01 00000 11111
    ppu->dot = 8;

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0u, ppu->v);     // 000 00 00000 00000
    // atb set to previous course-x selection
    ct_assertequal(2u, ppu->pxpl.tlu.atb);
}

static void fine_y_wraparound(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->v = 0x7065;                // 111 00 00011 00101
    ppu->dot = 256;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x86u, ppu->v);  // 000 00 00100 00110
    // atb set to previous course-x/y selection
    ct_assertequal(4u, ppu->pxpl.tlu.atb);
}

static void course_y_wraparound(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->v = 0x73a5;                // 111 00 11101 00101
    ppu->dot = 256;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x806u, ppu->v); // 000 10 00000 00110
    // atb set to previous course-x/y selection
    ct_assertequal(0u, ppu->pxpl.tlu.atb);
}

static void course_y_overflow(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->v = 0x73c5;                    // 111 00 11110 00101
    ppu->dot = 256;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x3e6u, ppu->v);     // 000 00 11111 00110
    // atb set to previous course-x/y selection
    ct_assertequal(4u, ppu->pxpl.tlu.atb);

    ppu->v = 0x73e6;                    // 111 00 11111 00110
    ppu->dot = 256;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(7u, ppu->v);         // 000 00 00000 00111
    // atb set to previous course-x/y selection
    ct_assertequal(6u, ppu->pxpl.tlu.atb);
}

//
// MARK: - Sprite Evaluation
//

static void secondary_oam_clear(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->dot = spr->oamd = spr->soaddr = 0;
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        spr->soam[i] = (uint8_t)(i + 1);
    }
    spr->s = ALDO_PPU_SPR_DONE;

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    for (auto i = 3; i < 65; ++i) {
        aldo_ppu_cycle(ppu);
    }
    ct_assertequal(65, ppu->dot);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(0xffu, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void secondary_oam_clear_with_offset_soamaddr(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->dot = spr->oamd = 0;
    spr->soaddr = 0x10;
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        spr->soam[i] = (uint8_t)(i + 1);
    }
    spr->s = ALDO_PPU_SPR_DONE;

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x11u, spr->soam[16]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0x10u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x11u, spr->soam[16]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0x10u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0xffu, spr->soam[16]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0x11u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    for (auto i = 3; i < 65; ++i) {
        aldo_ppu_cycle(ppu);
    }
    ct_assertequal(65, ppu->dot);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(0xffu, spr->soam[16]);
    ct_assertequal(0xffu, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void secondary_oam_does_not_clear_on_prerender_line(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 261;
    ppu->dot = spr->oamd = spr->soaddr = 0;
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        spr->soam[i] = (uint8_t)(i + 1);
    }
    spr->s = ALDO_PPU_SPR_DONE;

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    for (auto i = 3; i < 65; ++i) {
        aldo_ppu_cycle(ppu);
    }
    ct_assertequal(65, ppu->dot);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(1u, spr->soam[0]);
    ct_assertequal(0x20u, spr->soam[31]);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
}

static void sprite_below_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 19;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 20;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x33;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(50u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_above_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 28;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 20;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x33;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(50u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_top_on_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 20;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 20;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x23;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0xffu, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(69, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(70, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0xffu, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(71, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(72, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(0xffu, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(73, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(4u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(32u, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_bottom_on_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 27;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 20;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x23;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0xffu, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(69, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(70, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0xffu, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(71, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(72, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(0xffu, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(73, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(4u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(32u, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_sixteen_within_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;

    ppu->line = 28;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 20;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x23;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;
    ppu->ctrl.h = true;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0xffu, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(69, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(70, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0xffu, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(71, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(72, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(0xffu, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(73, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(4u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(32u, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_sixteen_bottom_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;

    ppu->line = 35;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 20;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x23;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;
    ppu->ctrl.h = true;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(1u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(1u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0xffu, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(69, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x22u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(70, ppu->dot);
    ct_assertequal(2u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(2u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0xffu, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(71, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(72, ppu->dot);
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(3u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(0xffu, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_COPY, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(73, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(32u, spr->oamd);
    ct_assertequal(4u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(0x22u, spr->soam[1]);
    ct_assertequal(0x23u, spr->soam[2]);
    ct_assertequal(32u, spr->soam[3]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_sixteen_above_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 36;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 20;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x33;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;
    ppu->ctrl.h = true;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(50u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(20u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_top_scanline_with_bottom_sprite(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 0xff;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x33;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(0xffu, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(50u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_bottom_scanline_with_top_sprite(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 239;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->oam[0] = 0;
    spr->oam[1] = 0x22;
    spr->oam[2] = 0x33;
    spr->oam[3] = 32;
    spr->oam[4] = 50;
    spr->soam[0] = spr->soam[1] = spr->soam[2] = spr->soam[3] = 0xff;
    spr->s = ALDO_PPU_SPR_SCAN;

    aldo_ppu_cycle(ppu);

    ct_assertequal(66, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0xffu, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(67, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);

    aldo_ppu_cycle(ppu);

    ct_assertequal(68, ppu->dot);
    ct_assertequal(4u, ppu->oamaddr);
    ct_assertequal(50u, spr->oamd);
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0u, spr->soam[0]);
    ct_assertequal(ALDO_PPU_SPR_SCAN, (int)spr->s);
}

static void sprite_evaluation_empty_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    for (size_t i = 0; i < aldo_arrsz(spr->oam) / 4; ++i) {
        // Set last sprite's y-coordinate uniquely to assert below that it
        // ends up in secondary OAM at the end.
        spr->oam[i * 4] = i == 63 ? 30 : 20;
    }
    memfill(spr->soam);

    // This should run through all 64 sprites, copying nothing but the
    // last sprite's Y coordinate to secondary OAM.
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_DONE);

    ct_assertequal(193, ppu->dot); // 2 dots per sprite = 128 dots
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(i == 0 ? 30u : 0xffu, spr->soam[i],
                       "unexpected value at soam idx %zu", i);
    }
    ct_assertequal(0u, spr->soaddr);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(30u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(i == 0 ? 30u : 0xffu, spr->soam[i],
                       "unexpected value at soam idx %zu", i);
    }
    ct_assertequal(0u, spr->soaddr);
    // For a completely missed scanline looks like oamaddr happens to
    // end up on the 33rd sprite at the end.
    ct_assertequal(0x80u, ppu->oamaddr);
    ct_assertequal(20u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
}

static void sprite_evaluation_partial_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    // add 4 sprites to this scanline
    spr->oam[4] = 12;
    spr->oam[5] = 0x10;
    spr->oam[6] = 0x3;
    spr->oam[7] = 0x12;
    spr->oam[12] = 10;
    spr->oam[13] = 0x20;
    spr->oam[14] = 0x23;
    spr->oam[15] = 0x22;
    spr->oam[32] = 8;
    spr->oam[33] = 0x30;
    spr->oam[34] = 0x43;
    spr->oam[35] = 0x32;
    spr->oam[40] = 11;
    spr->oam[41] = 0x40;
    spr->oam[42] = 0x63;
    spr->oam[43] = 0x42;
    spr->oam[63 * 4] = 80;  // unique y-coordinate of last sprite
    memfill(spr->soam);

    // this should run through all 64 sprites, copying 4 sprites into SOAM
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_DONE);

    // 128 dots + (2 dots for 3 attributes * 4 sprites = 24) = 152 dots
    ct_assertequal(217, ppu->dot);
    ct_assertequal(12, ppu->line);
    ct_assertequal(12u, spr->soam[0]);
    ct_assertequal(0x10u, spr->soam[1]);
    ct_assertequal(3u, spr->soam[2]);
    ct_assertequal(0x12u, spr->soam[3]);
    ct_assertequal(10u, spr->soam[4]);
    ct_assertequal(0x20u, spr->soam[5]);
    ct_assertequal(0x23u, spr->soam[6]);
    ct_assertequal(0x22u, spr->soam[7]);
    ct_assertequal(8u, spr->soam[8]);
    ct_assertequal(0x30u, spr->soam[9]);
    ct_assertequal(0x43u, spr->soam[10]);
    ct_assertequal(0x32u, spr->soam[11]);
    ct_assertequal(11u, spr->soam[12]);
    ct_assertequal(0x40u, spr->soam[13]);
    ct_assertequal(0x63u, spr->soam[14]);
    ct_assertequal(0x42u, spr->soam[15]);
    ct_assertequal(80u, spr->soam[16]);
    for (size_t i = 17; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(0xffu, spr->soam[i], "unexpected value at soam idx %zu", i);
    }
    ct_assertequal(0x10u, spr->soaddr);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(80u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(80u, spr->soam[16]);
    for (size_t i = 17; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(0xffu, spr->soam[i], "unexpected value at soam idx %zu", i);
    }
    ct_assertequal(0x10u, spr->soaddr);
    ct_assertequal(0x50u, ppu->oamaddr);    // ends up on the 21st sprite
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
}

static void sprite_evaluation_last_sprite_fills_scanline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    uint8_t sprites[] = {
        12, 0x10, 0x3, 0x12,
        10, 0x20, 0x23, 0x22,
        8, 0x30, 0x43, 0x32,
        11, 0x40, 0x63, 0x42,
        7, 0x50, 0x83, 0x52,
        6, 0x60, 0xa3, 0x62,
        9, 0x70, 0xc3, 0x72,
        12, 0x80, 0xe3, 0x82,
    };
    ct_assertequal(aldo_arrsz(sprites), aldo_arrsz(spr->soam));

    // add first seven sprites to oam
    memcpy(spr->oam, sprites, aldo_arrsz(sprites) - 4);
    // add last sprite to last oam slot
    spr->oam[252] = sprites[28];
    spr->oam[253] = sprites[29];
    spr->oam[254] = sprites[30];
    spr->oam[255] = sprites[31];
    memfill(spr->soam);

    // this should run through all 64 sprites, copying 8 sprites into SOAM
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_DONE);

    // 128 dots + (2 dots for 3 attributes * 8 sprites = 48) = 176 dots
    ct_assertequal(241, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0x82u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x20u, ppu->oamaddr);    // ends up on the 9th sprite
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
}

static void sprite_evaluation_fill_with_no_overflows(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    uint8_t sprites[] = {
        12, 0x10, 0x3, 0x12,
        10, 0x20, 0x23, 0x22,
        8, 0x30, 0x43, 0x32,
        11, 0x40, 0x63, 0x42,
        7, 0x50, 0x83, 0x52,
        6, 0x60, 0xa3, 0x62,
        9, 0x70, 0xc3, 0x72,
        12, 0x80, 0xe3, 0x82,
    };
    ct_assertequal(aldo_arrsz(sprites), aldo_arrsz(spr->soam));

    // Add 8 sprites in a row to some arbitrary aligned point in oam;
    // in this case the 7th sprite.
    memcpy(spr->oam + (6 * 4), sprites, aldo_arrsz(sprites));
    memfill(spr->soam);

    // stop after filling secondary OAM
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_FULL);

    // 6 sprite misses * 2 dots + 8 sprite hits * 8 dots = 76 dots
    ct_assertequal(141, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x38u, ppu->oamaddr);    // 14 sprites * 4 = OAMADDR + $38
    ct_assertequal(0x82u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);

    // start checking sprite overflow
    aldo_ppu_cycle(ppu);

    // read at OAMADDR
    ct_assertequal(142, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x38u, ppu->oamaddr);    // OAM[56][0]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // check overflow and increment (badly)
    ct_assertequal(143, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x3du, ppu->oamaddr);    // OAM[60][1]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    // continue diagonal walk of OAM with glitchy overflow checks
    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

    ct_assertequal(145, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x42u, ppu->oamaddr);    // OAM[64][2]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

    ct_assertequal(147, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x47u, ppu->oamaddr);    // OAM[68][3]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

    // overflowed attribute index m without carry, advancing OAMADDR by only 1
    ct_assertequal(149, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x48u, ppu->oamaddr);    // OAM[72][0]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

    // and continue with diagonal walk
    ct_assertequal(151, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x4du, ppu->oamaddr);    // OAM[76][1]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_DONE);

    ct_assertequal(241, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(2u, ppu->oamaddr);       // OAM[0][2]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x22u, ppu->oamaddr);    // ends up on the 9th sprite, attribute byte
    ct_assertequal(0x23u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_assertfalse(ppu->status.o);  // no overflow found
}

static void sprite_evaluation_next_sprite_overflows(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    // 9 sprites with overflow
    uint8_t sprites[] = {
        12, 0x10, 0x3, 0x12,
        10, 0x20, 0x23, 0x22,
        8, 0x30, 0x43, 0x32,
        11, 0x40, 0x63, 0x42,
        7, 0x50, 0x83, 0x52,
        6, 0x60, 0xa3, 0x62,
        9, 0x70, 0xc3, 0x72,
        12, 0x80, 0xe3, 0x82,
        10, 0x90, 0x81, 0x92,
    };

    memcpy(spr->oam, sprites, aldo_arrsz(sprites));
    memfill(spr->soam);

    // stop after filling secondary OAM
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_FULL);

    // 8 sprite hits * 8 dots = 64 dots
    ct_assertequal(129, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x20u, ppu->oamaddr);    // 8 sprites * 4 = OAMADDR + $20
    ct_assertequal(0x82u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);

    // start checking sprite overflow
    aldo_ppu_cycle(ppu);

    // read at OAMADDR
    ct_assertequal(130, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x20u, ppu->oamaddr);    // OAM[8][0]
    ct_assertequal(10u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // find overflow and read attributes
    ct_assertequal(131, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x21u, spr->soaddr);
    ct_assertequal(0x21u, ppu->oamaddr);    // OAM[8][1]
    ct_assertequal(10u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read overflow tile id
    ct_assertequal(132, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x21u, spr->soaddr);
    ct_assertequal(0x21u, ppu->oamaddr);    // OAM[8][1]
    ct_assertequal(0x90u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // ignore tile id and read soam
    ct_assertequal(133, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x22u, spr->soaddr);
    ct_assertequal(0x22u, ppu->oamaddr);    // OAM[8][2]
    ct_assertequal(0x90u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read overflow sprite attribute
    ct_assertequal(134, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x22u, spr->soaddr);
    ct_assertequal(0x22u, ppu->oamaddr);    // OAM[8][2]
    ct_assertequal(0x81u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // ignore sprite attribute and read soam
    ct_assertequal(135, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x23u, spr->soaddr);
    ct_assertequal(0x23u, ppu->oamaddr);    // OAM[8][3]
    ct_assertequal(0x81u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read overflow sprite x-coordinate
    ct_assertequal(136, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x23u, spr->soaddr);
    ct_assertequal(0x23u, ppu->oamaddr);    // OAM[8][3]
    ct_assertequal(0x92u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // ignore sprite attribute and read soam
    ct_assertequal(137, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x24u, spr->soaddr);
    ct_assertequal(0x24u, ppu->oamaddr);    // OAM[9][0]
    ct_assertequal(0x92u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    // overflow found, start idle scan of rest of sprites
    aldo_ppu_cycle(ppu);

    ct_assertequal(138, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x24u, spr->soaddr);
    ct_assertequal(0x24u, ppu->oamaddr);    // OAM[9][0]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    ct_assertequal(139, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x24u, spr->soaddr);
    ct_assertequal(0x28u, ppu->oamaddr);    // OAM[10][0]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    // finish scanning the sprites
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_DONE);

    ct_assertequal(247, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x24u, spr->soaddr);
    ct_assertequal(0u, ppu->oamaddr);       // OAM[0][0]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x24u, spr->soaddr);
    ct_assertequal(0x14u, ppu->oamaddr);    // ends up on the 6th sprite
    ct_assertequal(7u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_asserttrue(ppu->status.o);
}

static void sprite_evaluation_overflow_false_positive(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    // 9 sprites with false overflow
    uint8_t sprites[] = {
        12, 0x10, 0x3, 0x12,
        10, 0x20, 0x23, 0x22,
        8, 0x30, 0x43, 0x32,
        11, 0x40, 0x63, 0x42,
        7, 0x50, 0x83, 0x52,
        6, 0x60, 0xa3, 0x62,
        9, 0x70, 0xc3, 0x72,
        12, 0x80, 0xe3, 0x82,
        100, 9 /* interpreted as y-coord */, 0x81, 0x92,
    };

    memcpy(spr->oam, sprites, aldo_arrsz(sprites) - 4);
    // add last sprite one past the visible sprites at OAM[9][0]
    spr->oam[36] = sprites[32];
    spr->oam[37] = sprites[33];
    spr->oam[38] = sprites[34];
    spr->oam[39] = sprites[35];
    memfill(spr->soam);

    // stop after filling secondary OAM
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_FULL);

    // 8 sprite hits * 8 dots = 64 dots
    ct_assertequal(129, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x20u, ppu->oamaddr);    // 8 sprites * 4 = OAMADDR + $20
    ct_assertequal(0x82u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);

    // start checking sprite overflow at OAM[8][0]
    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

    ct_assertequal(131, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x25u, ppu->oamaddr);    // OAM[9][1]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read glitchy OAMADDR
    ct_assertequal(132, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x25u, ppu->oamaddr);    // OAM[9][1]
    ct_assertequal(9u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // set invalid overflow
    ct_assertequal(133, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x21u, spr->soaddr);
    ct_assertequal(0x26u, ppu->oamaddr);    // OAM[9][2]
    ct_assertequal(9u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read invalid overflow sprite attribute
    ct_assertequal(134, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x21u, spr->soaddr);
    ct_assertequal(0x26u, ppu->oamaddr);    // OAM[9][2]
    ct_assertequal(0x81u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // ignore sprite attribute and read soam
    ct_assertequal(135, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x22u, spr->soaddr);
    ct_assertequal(0x27u, ppu->oamaddr);    // OAM[9][3]
    ct_assertequal(0x81u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read overflow sprite x-coordinate
    ct_assertequal(136, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x22u, spr->soaddr);
    ct_assertequal(0x27u, ppu->oamaddr);    // OAM[9][3]
    ct_assertequal(0x92u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // ignore sprite attribute and read soam
    ct_assertequal(137, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x23u, spr->soaddr);
    ct_assertequal(0x28u, ppu->oamaddr);    // OAM[10][0]
    ct_assertequal(0x92u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    // read *next* sprite's y-coord as if it were the x-coord of previous sprite
    aldo_ppu_cycle(ppu);

    ct_assertequal(138, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x23u, spr->soaddr);
    ct_assertequal(0x28u, ppu->oamaddr);    // OAM[10][0]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    // ignore y-coord and read soam, end of overflow idle
    aldo_ppu_cycle(ppu);

    ct_assertequal(139, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x24u, spr->soaddr);
    ct_assertequal(0x29u, ppu->oamaddr);    // OAM[10][1]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x24u, spr->soaddr);
    ct_assertequal(0x15u, ppu->oamaddr);    // ends up on the 6th sprite, tile attribute
    ct_assertequal(0x50u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_asserttrue(ppu->status.o);
}

static void sprite_evaluation_overflow_false_negative(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    // 9 sprites with valid overflow that is missed
    uint8_t sprites[] = {
        12, 0x10, 0x3, 0x12,
        10, 0x20, 0x23, 0x22,
        8, 0x30, 0x43, 0x32,
        11, 0x40, 0x63, 0x42,
        7, 0x50, 0x83, 0x52,
        6, 0x60, 0xa3, 0x62,
        9, 0x70, 0xc3, 0x72,
        12, 0x80, 0xe3, 0x82,
        9 /* y-coord missed */, 0x90, 0x91, 0x92,
    };

    memcpy(spr->oam, sprites, aldo_arrsz(sprites) - 4);
    // add last sprite one past the visible sprites at OAM[9][0]
    spr->oam[36] = sprites[32];
    spr->oam[37] = sprites[33];
    spr->oam[38] = sprites[34];
    spr->oam[39] = sprites[35];
    memfill(spr->soam);

    // stop after filling secondary OAM
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_FULL);

    // 8 sprite hits * 8 dots = 64 dots
    ct_assertequal(129, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x20u, ppu->oamaddr);    // 8 sprites * 4 = OAMADDR + $20
    ct_assertequal(0x82u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);

    // start checking sprite overflow at OAM[8][0]
    aldo_ppu_cycle(ppu);
    aldo_ppu_cycle(ppu);

    ct_assertequal(131, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x25u, ppu->oamaddr);    // OAM[9][1]
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read glitchy OAMADDR
    ct_assertequal(132, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x25u, ppu->oamaddr);    // OAM[9][1]
    ct_assertequal(0x90u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // miss valid overflow
    ct_assertequal(133, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x2au, ppu->oamaddr);    // OAM[10][2]
    ct_assertequal(0x90u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x20u, ppu->oamaddr);    // ends up on the 9th sprite
    ct_assertequal(12u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_assertfalse(ppu->status.o);
}

static void sprite_evaluation_oam_overflow_during_sprite_overflow(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 37;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    // 9 sprites with valid overflow that is missed
    uint8_t sprites[] = {
        37, 0x10, 0x3, 0x12,
        35, 0x20, 0x23, 0x22,
        33, 0x30, 0x43, 0x32,
        36, 0x40, 0x63, 0x42,
        32, 0x50, 0x83, 0x52,
        31, 0x60, 0xa3, 0x62,
        34, 0x70, 0xc3, 0x72,
        37, 0x80, 0xe3, 0x82,
        100, 0x90, 35 /* overflow scan hits here for y-coord */, 0x92,
    };

    // offset sprites in OAM to get a favorable offset from the glitchy walk
    memcpy(spr->oam + 4, sprites, aldo_arrsz(sprites) - 4);
    // add last sprite to the end OAM[63][0]
    spr->oam[252] = sprites[32];
    spr->oam[253] = sprites[33];
    spr->oam[254] = sprites[34];
    spr->oam[255] = sprites[35];
    memfill(spr->soam);

    // stop after filling secondary OAM
    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_FULL);

    // 1 missed sprite * 2 dots + 8 sprite hits * 8 dots = 66 dots
    ct_assertequal(131, ppu->dot);
    ct_assertequal(37, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0x24u, ppu->oamaddr);
    ct_assertequal(0x82u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);

    // run to the last sprite
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->oamaddr < 252);

    ct_assertequal(239, ppu->dot);
    ct_assertequal(37, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0xfeu, ppu->oamaddr);    // OAM[63][2] due to glitchy overflow walk
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read OAM[63][2] as y-coordinate
    ct_assertequal(240, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x20u, spr->soaddr);
    ct_assertequal(0xfeu, ppu->oamaddr);    // OAM[63][2]
    ct_assertequal(35u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_FULL, (int)spr->s);
    ct_assertfalse(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // invalid overflow hit
    ct_assertequal(241, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x21u, spr->soaddr);
    ct_assertequal(0xffu, ppu->oamaddr);    // OAM[63][3]
    ct_assertequal(35u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // read OAM[63][3]
    ct_assertequal(242, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x21u, spr->soaddr);
    ct_assertequal(0xffu, ppu->oamaddr);    // OAM[63][3]
    ct_assertequal(0x92u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_OVER, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    aldo_ppu_cycle(ppu);

    // sprite overflow attribute scan interrupted by OAM overflow
    ct_assertequal(243, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x22u, spr->soaddr);
    ct_assertequal(0u, ppu->oamaddr);       // OAM[0][0]
    ct_assertequal(0x92u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_asserttrue(ppu->status.o);

    // run the rest of the sprite evaluation window
    do {
        aldo_ppu_cycle(ppu);
    } while (ppu->dot < 257);

    ct_assertequal(257, ppu->dot);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        ct_assertequal(sprites[i], spr->soam[i], "unexpected soam value at %zu", i);
    }
    ct_assertequal(0x22u, spr->soaddr);
    ct_assertequal(0x1cu, ppu->oamaddr);    // ends up on the 8th sprite
    ct_assertequal(31u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
    ct_asserttrue(ppu->status.o);
}

static void sprite_evaluation_oamaddr_offset(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 12;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    uint8_t sprites[] = {
        12, 0x10, 0x3, 0x12,
        10, 0x20, 0x23, 0x22,
        8, 0x30, 0x43, 0x32,
        11, 0x40, 0x63, 0x42,
        7, 0x50, 0x83, 0x52,
        6, 0x60, 0xa3, 0x62,
        9, 0x70, 0xc3, 0x72,
        12, 0x80, 0xe3, 0x82,
    };
    ct_assertequal(aldo_arrsz(sprites), aldo_arrsz(spr->soam));

    memcpy(spr->oam, sprites, aldo_arrsz(sprites));
    memfill(spr->soam);
    ppu->oamaddr = 0xc;    // start at OAM[3][0];

    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_DONE);

    ct_assertequal(217, ppu->dot);
    ct_assertequal(12, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        if (i < 20) {
            // first 3 sprites skipped by OAMADDR offset
            ct_assertequal(sprites[i + 12], spr->soam[i], "unexpected soam value at %zu", i);
        } else if (i == 20) {
            // last OAM read/write
            ct_assertequal(0u, spr->soam[i]);
        } else {
            ct_assertequal(0xffu, spr->soam[i]);
        }
    }
    ct_assertequal(0x14u, spr->soaddr); // secondary OAM contains 5 sprites
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
}

static void sprite_evaluation_oamaddr_misaligned(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    auto spr = &ppu->spr;
    ppu->line = 20;
    ppu->dot = 65;
    spr->oamd = spr->soaddr = 0;
    spr->s = ALDO_PPU_SPR_SCAN;
    aldo_memclr(spr->oam);
    uint8_t sprites[] = {
        12, 0x10, 0x3, 0x12,
        10, 0x20, 0x23, 0x22,
        8, 0x30, 0x43, 0x32,
        11, 0x40, 0x63, 0x42,
        7, 0x50, 0x83, 0x52,
        6, 0x60, 0xa3, 0x62,
        9, 0x70, 0xc3, 0x72,
        12, 0x80, 0xe3, 0x82,
    };
    ct_assertequal(aldo_arrsz(sprites), aldo_arrsz(spr->soam));

    memcpy(spr->oam, sprites, aldo_arrsz(sprites));
    memfill(spr->soam);
    ppu->oamaddr = 0x3;    // start at OAM[0][3];

    do {
        aldo_ppu_cycle(ppu);
    } while (spr->s != ALDO_PPU_SPR_DONE);

    ct_assertequal(199, ppu->dot);
    ct_assertequal(20, ppu->line);
    for (size_t i = 0; i < aldo_arrsz(spr->soam); ++i) {
        if (i < 4) {
            ct_assertequal(sprites[i + 3], spr->soam[i], "unexpected soam value at %zu", i);
        } else if (i == 4) {
            // last OAM read/write
            ct_assertequal(0u, spr->soam[i]);
        } else {
            ct_assertequal(0xffu, spr->soam[i]);
        }
    }
    ct_assertequal(4u, spr->soaddr);    // secondary OAM contains 1 sprite
    ct_assertequal(3u, ppu->oamaddr);
    ct_assertequal(0u, spr->oamd);
    ct_assertequal(ALDO_PPU_SPR_DONE, (int)spr->s);
}

static void oamaddr_cleared_during_sprite_fetch(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 256;
    ppu->oamaddr = 0x22;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x23u, ppu->oamaddr);

    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);

    ppu->oamaddr = 0x33;
    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);

    ppu->oamaddr = 0x44;
    ppu->dot = 321;
    aldo_ppu_cycle(ppu);

    ct_assertequal(322, ppu->dot);
    ct_assertequal(0x44u, ppu->oamaddr);
}

static void oamaddr_cleared_during_sprite_fetch_on_prerender(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 256;
    ppu->line = 261;
    ppu->oamaddr = 0x22;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x22u, ppu->oamaddr);

    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);

    ppu->oamaddr = 0x33;
    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0u, ppu->oamaddr);

    ppu->oamaddr = 0x44;
    ppu->dot = 321;
    aldo_ppu_cycle(ppu);

    ct_assertequal(322, ppu->dot);
    ct_assertequal(0x44u, ppu->oamaddr);
}

static void oamaddr_not_cleared_during_postrender(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 256;
    ppu->line = 240;
    ppu->oamaddr = 0x22;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x22u, ppu->oamaddr);

    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x22u, ppu->oamaddr);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x22u, ppu->oamaddr);
}

//
// MARK: - Pixel Pipeline
//

static void tile_prefetch_pipeline(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->pxpl.tlu.at = 0x56;
    ppu->pxpl.tlu.bg[0] = 0xbb;
    ppu->pxpl.tlu.bg[1] = 0xcc;
    ppu->line = 261;
    ppu->dot = 329;
    ppu->pxpl.tlu.atl[0] = true;
    ppu->pxpl.tlu.ats[0] = 0xff;

    aldo_ppu_cycle(ppu);

    ct_assertequal(330, ppu->dot);
    ct_assertfalse(ppu->pxpl.tlu.atl[0]);
    ct_asserttrue(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(0u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(0xbb00u, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0xcc00u, ppu->pxpl.tlu.bgs[1]);
    ct_assertfalse(ppu->signal.vout);

    aldo_ppu_cycle(ppu);

    // no changes because we don't emulate the prefetch shifts
    ct_assertequal(331, ppu->dot);
    ct_assertfalse(ppu->pxpl.tlu.atl[0]);
    ct_asserttrue(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(0u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(0xbb00u, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0xcc00u, ppu->pxpl.tlu.bgs[1]);
    ct_assertfalse(ppu->signal.vout);

    ppu->dot = 337;
    ppu->pxpl.tlu.at = 0xa9;
    ppu->pxpl.tlu.bg[0] = 0x66;
    ppu->pxpl.tlu.bg[1] = 0x55;
    aldo_ppu_cycle(ppu);

    ct_assertequal(338, ppu->dot);
    ct_asserttrue(ppu->pxpl.tlu.atl[0]);
    ct_assertfalse(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(0u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(0xbb66u, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0xcc55u, ppu->pxpl.tlu.bgs[1]);
    ct_assertfalse(ppu->signal.vout);
}

static void tile_prefetch_postrender(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->pxpl.tlu.bgs[0] = 0x1111;
    ppu->pxpl.tlu.bgs[1] = 0x2222;
    ppu->line = 250;
    ppu->dot = 329;
    ppu->pxpl.tlu.atl[1] = ppu->pxpl.tlu.atl[0] = true;
    ppu->pxpl.tlu.ats[0] = 0xee;
    ppu->pxpl.tlu.ats[1] = 0xdd;

    for (auto i = 0; i < 9; ++i) {
        aldo_ppu_cycle(ppu);

        ct_assertequal(329 + i + 1, ppu->dot);
        ct_asserttrue(ppu->pxpl.tlu.atl[0]);
        ct_asserttrue(ppu->pxpl.tlu.atl[1]);
        ct_assertequal(0xeeu, ppu->pxpl.tlu.ats[0]);
        ct_assertequal(0xddu, ppu->pxpl.tlu.ats[1]);
        ct_assertequal(0x1111u, ppu->pxpl.tlu.bgs[0]);
        ct_assertequal(0x2222u, ppu->pxpl.tlu.bgs[1]);
        ct_assertfalse(ppu->signal.vout);
    }
}

static void attribute_latch(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    uint16_t vs[] = {0x0, 0x2, 0x40, 0x42};
    ppu->pxpl.tlu.at = 0xe4;
    ppu->pxpl.tlu.atl[1] = ppu->pxpl.tlu.atl[0] = true;

    for (size_t i = 0; i < 4; ++i) {
        ppu->dot = 336;
        ppu->v = vs[i];
        aldo_ppu_cycle(ppu);
        ct_assertequal(i * 2, ppu->pxpl.tlu.atb);
        ct_assertequal((unsigned int)(vs[i] + 1), ppu->v);
        aldo_ppu_cycle(ppu);
        auto val = ppu->pxpl.tlu.atl[1] << 1 | ppu->pxpl.tlu.atl[0];
        ct_assertequal((int)i, val);
    }
}

static void first_pixel_bg(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->pxpl.tlu.bgs[0] = 0x6fff;
    ppu->pxpl.tlu.bgs[1] = 0xcfff;
    ppu->pxpl.tlu.ats[0] = 0x0;
    ppu->pxpl.tlu.ats[1] = 0xff;

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.mux);
    ct_assertequal(0u, ppu->pxpl.pal);
    ct_assertequal(0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // Pipeline Idle
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0u, ppu->pxpl.mux);
    ct_assertequal(0u, ppu->pxpl.pal);
    ct_assertequal(0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // First Mux-and-Shift
    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0xau, ppu->pxpl.mux);
    ct_assertequal(0u, ppu->pxpl.pal);
    ct_assertequal(0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // Set Palette Address
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0xbu, ppu->pxpl.mux);
    ct_assertequal(0xau, ppu->pxpl.pal);
    ct_assertequal(0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // First Pixel Output
    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(9u, ppu->pxpl.mux);
    ct_assertequal(0xbu, ppu->pxpl.pal);
    ct_assertequal(6u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

static void last_pixel_bg(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 256;
    ppu->pxpl.tlu.bgs[0] = 0xb7ff;
    ppu->pxpl.tlu.bgs[1] = 0x67ff;
    ppu->pxpl.tlu.atl[0] = false;
    ppu->pxpl.tlu.atl[1] = true;
    ppu->pxpl.tlu.ats[0] = 0x0;
    ppu->pxpl.tlu.ats[1] = 0xff;
    ppu->pxpl.mux = 0x1;
    ppu->pxpl.pal = 0x3;

    // Final Tile Fetch Cycle (unused)
    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x6fffu, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0xcfffu, ppu->pxpl.tlu.bgs[1]);
    ct_assertfalse(ppu->pxpl.tlu.atl[0]);
    ct_asserttrue(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(0u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(9u, ppu->pxpl.mux);
    ct_assertequal(1u, ppu->pxpl.pal);
    ct_assertequal(2u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    ppu->pxpl.tlu.bg[0] = 0x11;
    ppu->pxpl.tlu.bg[1] = 0x22;
    ppu->pxpl.tlu.at = 0x1;

    // Last Full Mux-and-Shift
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0xdf11u, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0x9f22u, ppu->pxpl.tlu.bgs[1]);
    ct_asserttrue(ppu->pxpl.tlu.atl[0]);
    ct_assertfalse(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(0u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(0xau, ppu->pxpl.mux);
    ct_assertequal(9u, ppu->pxpl.pal);
    ct_assertequal(0u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    // Set Palette Address
    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0xbe23u, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0x3e45u, ppu->pxpl.tlu.bgs[1]);
    ct_asserttrue(ppu->pxpl.tlu.atl[0]);
    ct_assertfalse(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(1u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xfeu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(0xbu, ppu->pxpl.mux);
    ct_assertequal(0xau, ppu->pxpl.pal);
    ct_assertequal(5u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    // Last Pixel Output
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x7c47u, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0x7c8bu, ppu->pxpl.tlu.bgs[1]);
    ct_asserttrue(ppu->pxpl.tlu.atl[0]);
    ct_assertfalse(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(3u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xfcu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(9u, ppu->pxpl.mux);
    ct_assertequal(0xbu, ppu->pxpl.pal);
    ct_assertequal(6u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    // Video Out Stops, Pipeline Idle
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x7c47u, ppu->pxpl.tlu.bgs[0]);
    ct_assertequal(0x7c8bu, ppu->pxpl.tlu.bgs[1]);
    ct_asserttrue(ppu->pxpl.tlu.atl[0]);
    ct_assertfalse(ppu->pxpl.tlu.atl[1]);
    ct_assertequal(3u, ppu->pxpl.tlu.ats[0]);
    ct_assertequal(0xfcu, ppu->pxpl.tlu.ats[1]);
    ct_assertequal(9u, ppu->pxpl.mux);
    ct_assertequal(0xbu, ppu->pxpl.pal);
    ct_assertequal(6u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);
}

static void pixel_postrender_bg(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 250;
    ppu->dot = 10;
    ppu->pxpl.mux = 0x4;
    ppu->pxpl.pal = 0x5;
    ppu->pxpl.px = 0x7;

    for (auto i = 0; i < 9; ++i) {
        aldo_ppu_cycle(ppu);

        ct_assertequal(10 + i + 1, ppu->dot);
        ct_assertequal(4u, ppu->pxpl.mux);
        ct_assertequal(5u, ppu->pxpl.pal);
        ct_assertequal(7u, ppu->pxpl.px);
        ct_assertfalse(ppu->signal.vout);
    }
}

static void pixel_transparent_bg(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.tlu.bgs[0] = 0x1fff;
    ppu->pxpl.tlu.bgs[1] = 0x1fff;
    ppu->pxpl.tlu.ats[0] = 0xbf;
    ppu->pxpl.tlu.ats[1] = 0x7f;
    ppu->pxpl.mux = 0x0;
    ppu->pxpl.pal = 0x6;

    aldo_ppu_cycle(ppu);

    ct_assertequal(4u, ppu->pxpl.mux);
    ct_assertequal(0u, ppu->pxpl.pal);
    ct_assertequal(0x11u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8u, ppu->pxpl.mux);
    ct_assertequal(0u, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xcu, ppu->pxpl.mux);
    ct_assertequal(0u, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xfu, ppu->pxpl.mux);
    ct_assertequal(0u, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xfu, ppu->pxpl.mux);
    ct_assertequal(0xfu, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);
}

static void pixel_disabled_bg(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.tlu.bgs[1] = ppu->pxpl.tlu.bgs[0] = 0xffff;
    ppu->pxpl.tlu.ats[1] = ppu->pxpl.tlu.ats[0] = 0xff;
    ppu->mask.b = false;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xcu, ppu->pxpl.mux);
}

static void left_mask_bg(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->line = 10;
    ppu->dot = 2;
    ppu->pxpl.tlu.atl[1] = ppu->pxpl.tlu.atl[0] = true;
    ppu->pxpl.tlu.ats[1] = ppu->pxpl.tlu.ats[0] = ppu->pxpl.tlu.bg[1] =
        ppu->pxpl.tlu.bg[0] = 0xff;
    ppu->pxpl.tlu.bgs[1] = ppu->pxpl.tlu.bgs[0] = 0xffff;
    ppu->mask.bm = false;

    for (auto i = 0; i < 8; ++i) {
        aldo_ppu_cycle(ppu);

        ct_assertequal(2 + i + 1, ppu->dot);
        ct_assertequal(0xcu, ppu->pxpl.mux);
    }

    aldo_ppu_cycle(ppu);

    ct_assertequal(11, ppu->dot);
    ct_assertequal(0xfu, ppu->pxpl.mux);
}

static void fine_x_select(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);

    uint8_t bg0 = 0, bg1 = 0, at0 = 0, at1 = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        ppu->dot = 70;
        ppu->pxpl.tlu.bgs[0] = 0xcd00;
        ppu->pxpl.tlu.bgs[1] = 0xa800;
        ppu->pxpl.tlu.ats[0] = 0x73;
        ppu->pxpl.tlu.ats[1] = 0x41;
        ppu->x = i;

        aldo_ppu_cycle(ppu);

        bg0 |= (aldo_getbit(ppu->pxpl.mux, 0) << i);
        bg1 |= (aldo_getbit(ppu->pxpl.mux, 1) << i);
        at0 |= (aldo_getbit(ppu->pxpl.mux, 2) << i);
        at1 |= (aldo_getbit(ppu->pxpl.mux, 3) << i);
    }
    ct_assertequal(0xb3u, bg0);  // Reverse of 0xCD
    ct_assertequal(0x15u, bg1);  // Reverse of 0xA8
    ct_assertequal(0xceu, at0);  // Reverse of 0x73
    ct_assertequal(0x82u, at1);  // Reverse of 0x41
}

//
// MARK: - Rendering Disabled
//

static void rendering_disabled(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.mux = 0xf;
    ppu->mask.s = ppu->mask.b = false;
    ppu->v = 0x20ff;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->pxpl.pal);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x24u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

static void rendering_disabled_explicit_bg_palette(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.mux = 0xf;
    ppu->mask.s = ppu->mask.b = false;
    ppu->v = 0x3f09;

    aldo_ppu_cycle(ppu);

    ct_assertequal(9u, ppu->pxpl.pal);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

static void rendering_disabled_unused_bg_palette(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.mux = 0xf;
    ppu->mask.s = ppu->mask.b = false;
    ppu->v = 0x3f0c;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xcu, ppu->pxpl.pal);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xdu, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

static void rendering_disabled_explicit_fg_palette(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.mux = 0xf;
    ppu->mask.s = ppu->mask.b = false;
    ppu->v = 0x3f19;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x19u, ppu->pxpl.pal);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x33u, ppu->pxpl.px);    // 0x3f19 points at pal[22];
    ct_asserttrue(ppu->signal.vout);
}

static void rendering_disabled_unused_fg_palette(void *ctx)
{
    auto ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.mux = 0xf;
    ppu->mask.s = ppu->mask.b = false;
    ppu->v = 0x3f1c;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x1cu, ppu->pxpl.pal);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xdu, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

//
// MARK: - Test List
//

struct ct_testsuite ppu_render_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(nametable_fetch),
        ct_maketest(attributetable_fetch),
        ct_maketest(tile_fetch),
        ct_maketest(tile_fetch_higher_bits_sequence),
        ct_maketest(tile_fetch_post_render_line),
        ct_maketest(tile_fetch_rendering_disabled),
        ct_maketest(tile_fetch_tile_only_disabled),

        ct_maketest(render_line_end),
        ct_maketest(render_line_prefetch),
        ct_maketest(prerender_nametable_fetch),
        ct_maketest(prerender_end_even_frame),
        ct_maketest(prerender_end_odd_frame),
        ct_maketest(prerender_set_vertical_coords),
        ct_maketest(render_not_set_vertical_coords),

        ct_maketest(sprite_fetch_first_row),
        ct_maketest(sprite_fetch_last_row),
        ct_maketest(sprite_fetch_vertical_flip),
        ct_maketest(sprite_fetch_horizontal_flip),
        ct_maketest(sprite_fetch_vertical_and_horizontal_flip),
        ct_maketest(sprite_fetch_upper_bank),
        ct_maketest(sprite_dh_fetch_top_first_row),
        ct_maketest(sprite_dh_fetch_top_last_row),
        ct_maketest(sprite_dh_fetch_bottom_first_row),
        ct_maketest(sprite_dh_fetch_bottom_last_row),
        ct_maketest(sprite_dh_fetch_vertical_flip_top_first_row),
        ct_maketest(sprite_dh_fetch_vertical_flip_top_last_row),
        ct_maketest(sprite_dh_fetch_vertical_flip_bottom_first_row),
        ct_maketest(sprite_dh_fetch_vertical_flip_bottom_last_row),
        ct_maketest(sprite_dh_fetch_upper_bank),
        ct_maketest(sprite_fetch_out_of_range_sprite),
        ct_maketest(sprite_fetch_rendering_disabled),
        ct_maketest(sprite_fetch_sprite_only_disabled),
        ct_maketest(sprite_fetch_full_line_sequence),
        ct_maketest(sprite_fetch_empty_line_sequence),
        ct_maketest(sprite_fetch_partial_line_sequence),
        ct_maketest(sprite_fetch_starts_late),

        ct_maketest(course_x_wraparound),
        ct_maketest(fine_y_wraparound),
        ct_maketest(course_y_wraparound),
        ct_maketest(course_y_overflow),

        ct_maketest(secondary_oam_clear),
        ct_maketest(secondary_oam_clear_with_offset_soamaddr),
        ct_maketest(secondary_oam_does_not_clear_on_prerender_line),
        ct_maketest(sprite_below_scanline),
        ct_maketest(sprite_above_scanline),
        ct_maketest(sprite_top_on_scanline),
        ct_maketest(sprite_bottom_on_scanline),
        ct_maketest(sprite_sixteen_within_scanline),
        ct_maketest(sprite_sixteen_bottom_scanline),
        ct_maketest(sprite_sixteen_above_scanline),
        ct_maketest(sprite_top_scanline_with_bottom_sprite),
        ct_maketest(sprite_bottom_scanline_with_top_sprite),
        ct_maketest(sprite_evaluation_empty_scanline),
        ct_maketest(sprite_evaluation_partial_scanline),
        ct_maketest(sprite_evaluation_last_sprite_fills_scanline),
        ct_maketest(sprite_evaluation_fill_with_no_overflows),
        ct_maketest(sprite_evaluation_next_sprite_overflows),
        ct_maketest(sprite_evaluation_overflow_false_positive),
        ct_maketest(sprite_evaluation_overflow_false_negative),
        ct_maketest(sprite_evaluation_oam_overflow_during_sprite_overflow),
        ct_maketest(sprite_evaluation_oamaddr_offset),
        ct_maketest(sprite_evaluation_oamaddr_misaligned),
        ct_maketest(oamaddr_cleared_during_sprite_fetch),
        ct_maketest(oamaddr_cleared_during_sprite_fetch_on_prerender),
        ct_maketest(oamaddr_not_cleared_during_postrender),

        ct_maketest(tile_prefetch_pipeline),
        ct_maketest(tile_prefetch_postrender),
        ct_maketest(attribute_latch),
        ct_maketest(first_pixel_bg),
        ct_maketest(last_pixel_bg),
        ct_maketest(pixel_postrender_bg),
        ct_maketest(pixel_transparent_bg),
        ct_maketest(pixel_disabled_bg),
        ct_maketest(left_mask_bg),
        ct_maketest(fine_x_select),

        ct_maketest(rendering_disabled),
        ct_maketest(rendering_disabled_explicit_bg_palette),
        ct_maketest(rendering_disabled_unused_bg_palette),
        ct_maketest(rendering_disabled_explicit_fg_palette),
        ct_maketest(rendering_disabled_unused_fg_palette),
    };

    return ct_makesuite_setup_teardown(tests, ppu_render_setup, ppu_render_teardown);
}
