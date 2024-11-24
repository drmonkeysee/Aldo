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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint8_t
        NameTables[2][8],
        AttributeTables[2][8],
        PatternTables[2][16];

static bool chrread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    if (addr < ALDO_MEMBLOCK_8KB) {
        uint8_t *ptable = addr < 0x1000 ? PatternTables[0] : PatternTables[1];
        *d = ptable[addr % 0x10];
        return true;
    }
    return false;
}

static bool vramread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    if (ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB) {
        // NOTE: use horizontal mirroring for testing (Donkey Kong setting)
        size_t select = addr < 0x2800 ? 0 : 1;
        *d = (addr & 0x3ff) < 0x3c0
                ? NameTables[select][addr % 0x8]
                : AttributeTables[select][addr % 0x8];
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
    // NOTE: turn on rendering and turn left-column mask off
    c->ppu.mask.b = c->ppu.mask.s = c->ppu.mask.bm = c->ppu.mask.sm = true;
    *ctx = v;
}

//
// MARK: - Basic Tile Fetches
//

static void nametable_fetch(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 1;  // NOTE: avoid odd-frame skipped-dot behavior on line 0

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void attributetable_fetch(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    AttributeTables[0][1] = 0x22;
    ppu->v = 0x5;
    ppu->dot = 3;

    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void tile_fetch(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->dot = 5;
    ppu->pxpl.nt = 0x11;

    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x6u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void tile_fetch_higher_bits_sequence(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[1][2] = 0x11;
    AttributeTables[1][2] = 0x22;
    PatternTables[1][3] = 0x33;
    PatternTables[1][11] = 0x44;
    ppu->v = 0x3aea;
    ppu->ctrl.b = true;
    ppu->line = 1;  // NOTE: avoid odd-frame skipped-dot behavior on line 0

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x1003u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x3aebu, ppu->v);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void tile_fetch_post_render_line(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
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
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void tile_fetch_rendering_disabled(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[1][2] = 0x11;
    AttributeTables[1][2] = 0x22;
    PatternTables[1][3] = 0x33;
    PatternTables[1][11] = 0x44;
    ppu->v = 0x3aea;
    ppu->ctrl.b = true;
    ppu->line = 1;  // NOTE: avoid odd-frame skipped-dot behavior on line 0
    ppu->mask.b = ppu->mask.s = false;

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void tile_fetch_tile_only_disabled(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[1][2] = 0x11;
    AttributeTables[1][2] = 0x22;
    PatternTables[1][3] = 0x33;
    PatternTables[1][11] = 0x44;
    ppu->v = 0x3aea;
    ppu->ctrl.b = true;
    ppu->line = 1;  // NOTE: avoid odd-frame skipped-dot behavior on line 0
    ppu->mask.b = false;

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x1003u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2aeau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x2beau, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.at);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(6, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(7, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x1113u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(8, ppu->dot);
    ct_assertequal(0x3aeau, ppu->v);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x3aebu, ppu->v);
    ct_assertequal(0x111bu, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

//
// MARK: - Line Edge Cases
//

static void render_line_end(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[0][3] = 0x99;
    NameTables[0][6] = 0x77;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->t = 0x7b3;
    ppu->dot = 255;
    ppu->pxpl.nt = 0x11;

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(256, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x0u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x1006u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Garbage NT Fetches
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0x77u, ppu->pxpl.nt);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x77u, ppu->vdatabus);
    ct_assertequal(0x77u, ppu->pxpl.nt);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x1413u, ppu->v);
    ct_assertequal(0x2413u, ppu->vaddrbus);
    ct_assertequal(0x99u, ppu->vdatabus);
    ct_assertequal(0x99u, ppu->pxpl.nt);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void render_line_prefetch(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    NameTables[0][6] = 0xaa;
    NameTables[0][7] = 0xee;
    AttributeTables[0][1] = 0x22;
    PatternTables[0][0] = 0x33;
    PatternTables[0][8] = 0x44;
    ppu->v = 0x5;
    ppu->dot = 321;

    // Tile Fetch 1
    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(322, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(323, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(324, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.at);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(325, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.at);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(326, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(327, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(328, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0u, ppu->pxpl.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(329, ppu->dot);
    ct_assertequal(0x6u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch 2
    // NT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(330, ppu->dot);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(331, ppu->dot);
    ct_assertequal(0x2006u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0xaau, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // AT Fetch
    aldo_ppu_cycle(ppu);

    ct_assertequal(332, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0xaau, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.at);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(333, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->pxpl.at);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Tile Fetch
    // NOTE: simulate getting a different tile
    PatternTables[0][0] = 0xbb;
    PatternTables[0][8] = 0xcc;

    aldo_ppu_cycle(ppu);

    ct_assertequal(334, ppu->dot);
    ct_assertequal(0x6u, ppu->v);
    ct_assertequal(0x0aa0u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->pxpl.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(335, ppu->dot);
    ct_assertequal(0x6u, ppu->v);
    ct_assertequal(0x0aa0u, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(336, ppu->dot);
    ct_assertequal(0x6u, ppu->v);
    ct_assertequal(0x0aa8u, ppu->vaddrbus);
    ct_assertequal(0xbbu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.bg[0]);
    ct_assertequal(0x44u, ppu->pxpl.bg[1]);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(337, ppu->dot);
    ct_assertequal(0x7u, ppu->v);
    ct_assertequal(0x0aa8u, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.bg[0]);
    ct_assertequal(0xccu, ppu->pxpl.bg[1]);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Garbage NT Fetches
    aldo_ppu_cycle(ppu);

    ct_assertequal(338, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xccu, ppu->vdatabus);
    ct_assertequal(0xaau, ppu->pxpl.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(339, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xeeu, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(340, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xeeu, ppu->pxpl.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0, ppu->dot);
    ct_assertequal(0x2007u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xeeu, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->line);
    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x7u, ppu->v);
    ct_assertequal(0x0ee0u, ppu->vaddrbus);
    ct_assertequal(0xeeu, ppu->vdatabus);
    ct_assertequal(0xbbu, ppu->pxpl.bg[0]);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void prerender_nametable_fetch(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 261;

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
}

static void prerender_end_even_frame(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 261;
    ppu->dot = 339;

    aldo_ppu_cycle(ppu);

    ct_assertequal(340, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertfalse(ppu->odd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
    ct_assertequal(0, ppu->line);
    ct_asserttrue(ppu->odd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
}

static void prerender_end_odd_frame(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;
    ppu->line = 261;
    ppu->dot = 339;
    ppu->odd = true;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->pxpl.nt);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);
    ct_assertfalse(ppu->odd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->pxpl.nt);
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
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->v = 0x5;
    ppu->t = 0x5fba;
    ppu->dot = 279;
    ppu->line = 261;

    aldo_ppu_cycle(ppu);

    ct_assertequal(280, ppu->dot);
    ct_assertequal(0x5u, ppu->v);

    while (ppu->dot < 305) {
        aldo_ppu_cycle(ppu);

        ct_assertequal(0x5ba5u, ppu->v);
        ppu->v = 0x5;
    }

    aldo_ppu_cycle(ppu);

    ct_assertequal(306, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
}

static void render_not_set_vertical_coords(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->v = 0x5;
    ppu->t = 0x5fba;
    ppu->dot = 279;

    aldo_ppu_cycle(ppu);

    ct_assertequal(280, ppu->dot);
    ct_assertequal(0x5u, ppu->v);

    while (ppu->dot < 305) {
        aldo_ppu_cycle(ppu);

        ct_assertequal(0x5u, ppu->v);
        ppu->v = 0x5;
    }

    aldo_ppu_cycle(ppu);

    ct_assertequal(306, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
}

//
// MARK: - Coordinate Behaviors
//

static void course_x_wraparound(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->v = 0x1f;                  // 000 00 00000 11111
    ppu->dot = 8;

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x400u, ppu->v); // 000 01 00000 00000

    ppu->v = 0x41f;                 // 000 01 00000 11111
    ppu->dot = 8;

    aldo_ppu_cycle(ppu);

    ct_assertequal(9, ppu->dot);
    ct_assertequal(0x0u, ppu->v);   // 000 00 00000 00000
}

static void fine_y_wraparound(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->v = 0x7065;                // 111 00 00011 00101
    ppu->dot = 256;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x86u, ppu->v);  // 000 00 00100 00110
}

static void course_y_wraparound(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->v = 0x73a5;                // 111 00 11101 00101
    ppu->dot = 256;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x806u, ppu->v); // 000 10 00000 00110
}

static void course_y_overflow(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->v = 0x73c5;                // 111 00 11110 00101
    ppu->dot = 256;

    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x6u, ppu->v);   // 000 00 00000 00110
}

//
// MARK: - Pixel Pipeline
//

static void tile_prefetch_pipeline(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->pxpl.at = 0x56;
    ppu->pxpl.bg[0] = 0xbb;
    ppu->pxpl.bg[1] = 0xcc;
    ppu->line = 261;
    ppu->dot = 329;
    ppu->pxpl.atl[0] = true;
    ppu->pxpl.ats[0] = 0xff;

    aldo_ppu_cycle(ppu);

    ct_assertequal(330, ppu->dot);
    ct_assertfalse(ppu->pxpl.atl[0]);
    ct_asserttrue(ppu->pxpl.atl[1]);
    ct_assertequal(0x0u, ppu->pxpl.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.ats[1]);
    ct_assertequal(0xbb00u, ppu->pxpl.bgs[0]);
    ct_assertequal(0xcc00u, ppu->pxpl.bgs[1]);
    ct_assertfalse(ppu->signal.vout);

    aldo_ppu_cycle(ppu);

    // no changes because we don't emulate the prefetch shifts
    ct_assertequal(331, ppu->dot);
    ct_assertfalse(ppu->pxpl.atl[0]);
    ct_asserttrue(ppu->pxpl.atl[1]);
    ct_assertequal(0x0u, ppu->pxpl.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.ats[1]);
    ct_assertequal(0xbb00u, ppu->pxpl.bgs[0]);
    ct_assertequal(0xcc00u, ppu->pxpl.bgs[1]);
    ct_assertfalse(ppu->signal.vout);

    ppu->dot = 337;
    ppu->pxpl.at = 0xa9;
    ppu->pxpl.bg[0] = 0x66;
    ppu->pxpl.bg[1] = 0x55;
    aldo_ppu_cycle(ppu);

    ct_assertequal(338, ppu->dot);
    ct_asserttrue(ppu->pxpl.atl[0]);
    ct_assertfalse(ppu->pxpl.atl[1]);
    ct_assertequal(0x0u, ppu->pxpl.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.ats[1]);
    ct_assertequal(0xbb66u, ppu->pxpl.bgs[0]);
    ct_assertequal(0xcc55u, ppu->pxpl.bgs[1]);
    ct_assertfalse(ppu->signal.vout);
}

static void tile_prefetch_postrender(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->pxpl.bgs[0] = 0x1111;
    ppu->pxpl.bgs[1] = 0x2222;
    ppu->line = 250;
    ppu->dot = 329;
    ppu->pxpl.atl[1] = ppu->pxpl.atl[0] = true;
    ppu->pxpl.ats[0] = 0xee;
    ppu->pxpl.ats[1] = 0xdd;

    for (int i = 0; i < 9; ++i) {
        aldo_ppu_cycle(ppu);

        ct_assertequal(329 + i + 1, ppu->dot);
        ct_asserttrue(ppu->pxpl.atl[0]);
        ct_asserttrue(ppu->pxpl.atl[1]);
        ct_assertequal(0xeeu, ppu->pxpl.ats[0]);
        ct_assertequal(0xddu, ppu->pxpl.ats[1]);
        ct_assertequal(0x1111u, ppu->pxpl.bgs[0]);
        ct_assertequal(0x2222u, ppu->pxpl.bgs[1]);
        ct_assertfalse(ppu->signal.vout);
    }
}

static void attribute_latch(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    uint16_t vs[] = {0x0, 0x2, 0x40, 0x42};
    ppu->pxpl.at = 0xe4;
    ppu->pxpl.atl[1] = ppu->pxpl.atl[0] = true;

    for (size_t i = 0; i < 4; ++i) {
        ppu->dot = 337;
        ppu->v = vs[i];
        aldo_ppu_cycle(ppu);
        int val = ppu->pxpl.atl[1] << 1 | ppu->pxpl.atl[0];
        ct_assertequal((int)i, val);
    }
}

static void first_pixel_bg(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->pxpl.bgs[0] = 0x6fff;
    ppu->pxpl.bgs[1] = 0xcfff;
    ppu->pxpl.ats[0] = 0x0;
    ppu->pxpl.ats[1] = 0xff;

    // Idle Cycle
    aldo_ppu_cycle(ppu);

    ct_assertequal(1, ppu->dot);
    ct_assertequal(0x0u, ppu->pxpl.mux);
    ct_assertequal(0x0u, ppu->pxpl.pal);
    ct_assertequal(0x0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // Pipeline Idle
    aldo_ppu_cycle(ppu);

    ct_assertequal(2, ppu->dot);
    ct_assertequal(0x0u, ppu->pxpl.mux);
    ct_assertequal(0x0u, ppu->pxpl.pal);
    ct_assertequal(0x0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // First Mux-and-Shift
    aldo_ppu_cycle(ppu);

    ct_assertequal(3, ppu->dot);
    ct_assertequal(0xau, ppu->pxpl.mux);
    ct_assertequal(0x0u, ppu->pxpl.pal);
    ct_assertequal(0x0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // Set Palette Address
    aldo_ppu_cycle(ppu);

    ct_assertequal(4, ppu->dot);
    ct_assertequal(0xbu, ppu->pxpl.mux);
    ct_assertequal(0xau, ppu->pxpl.pal);
    ct_assertequal(0x0u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);

    // First Pixel Output
    aldo_ppu_cycle(ppu);

    ct_assertequal(5, ppu->dot);
    ct_assertequal(0x9u, ppu->pxpl.mux);
    ct_assertequal(0xbu, ppu->pxpl.pal);
    ct_assertequal(0x6u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

static void last_pixel_bg(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->dot = 256;
    ppu->pxpl.bgs[0] = 0xb7ff;
    ppu->pxpl.bgs[1] = 0x67ff;
    ppu->pxpl.atl[0] = false;
    ppu->pxpl.atl[1] = true;
    ppu->pxpl.ats[0] = 0x0;
    ppu->pxpl.ats[1] = 0xff;
    ppu->pxpl.mux = 0x1;
    ppu->pxpl.pal = 0x3;

    // Final Tile Fetch Cycle (unused)
    aldo_ppu_cycle(ppu);

    ct_assertequal(257, ppu->dot);
    ct_assertequal(0x6fffu, ppu->pxpl.bgs[0]);
    ct_assertequal(0xcfffu, ppu->pxpl.bgs[1]);
    ct_assertfalse(ppu->pxpl.atl[0]);
    ct_asserttrue(ppu->pxpl.atl[1]);
    ct_assertequal(0x0u, ppu->pxpl.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.ats[1]);
    ct_assertequal(0x9u, ppu->pxpl.mux);
    ct_assertequal(0x1u, ppu->pxpl.pal);
    ct_assertequal(0x2u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    ppu->pxpl.bg[0] = 0x11;
    ppu->pxpl.bg[1] = 0x22;
    ppu->pxpl.at = 0x1;

    // Last Full Mux-and-Shift
    aldo_ppu_cycle(ppu);

    ct_assertequal(258, ppu->dot);
    ct_assertequal(0xdf11u, ppu->pxpl.bgs[0]);
    ct_assertequal(0x9f22u, ppu->pxpl.bgs[1]);
    ct_asserttrue(ppu->pxpl.atl[0]);
    ct_assertfalse(ppu->pxpl.atl[1]);
    ct_assertequal(0x0u, ppu->pxpl.ats[0]);
    ct_assertequal(0xffu, ppu->pxpl.ats[1]);
    ct_assertequal(0xau, ppu->pxpl.mux);
    ct_assertequal(0x9u, ppu->pxpl.pal);
    ct_assertequal(0x0u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    // Set Palette Address
    aldo_ppu_cycle(ppu);

    ct_assertequal(259, ppu->dot);
    ct_assertequal(0xbe23u, ppu->pxpl.bgs[0]);
    ct_assertequal(0x3e45u, ppu->pxpl.bgs[1]);
    ct_asserttrue(ppu->pxpl.atl[0]);
    ct_assertfalse(ppu->pxpl.atl[1]);
    ct_assertequal(0x1u, ppu->pxpl.ats[0]);
    ct_assertequal(0xfeu, ppu->pxpl.ats[1]);
    ct_assertequal(0xbu, ppu->pxpl.mux);
    ct_assertequal(0xau, ppu->pxpl.pal);
    ct_assertequal(0x5u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    // Last Pixel Output
    aldo_ppu_cycle(ppu);

    ct_assertequal(260, ppu->dot);
    ct_assertequal(0x7c47u, ppu->pxpl.bgs[0]);
    ct_assertequal(0x7c8bu, ppu->pxpl.bgs[1]);
    ct_asserttrue(ppu->pxpl.atl[0]);
    ct_assertfalse(ppu->pxpl.atl[1]);
    ct_assertequal(0x3u, ppu->pxpl.ats[0]);
    ct_assertequal(0xfcu, ppu->pxpl.ats[1]);
    ct_assertequal(0x9u, ppu->pxpl.mux);
    ct_assertequal(0xbu, ppu->pxpl.pal);
    ct_assertequal(0x6u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);

    // Video Out Stops, Pipeline Idle
    aldo_ppu_cycle(ppu);

    ct_assertequal(261, ppu->dot);
    ct_assertequal(0x7c47u, ppu->pxpl.bgs[0]);
    ct_assertequal(0x7c8bu, ppu->pxpl.bgs[1]);
    ct_asserttrue(ppu->pxpl.atl[0]);
    ct_assertfalse(ppu->pxpl.atl[1]);
    ct_assertequal(0x3u, ppu->pxpl.ats[0]);
    ct_assertequal(0xfcu, ppu->pxpl.ats[1]);
    ct_assertequal(0x9u, ppu->pxpl.mux);
    ct_assertequal(0xbu, ppu->pxpl.pal);
    ct_assertequal(0x6u, ppu->pxpl.px);
    ct_assertfalse(ppu->signal.vout);
}

static void pixel_postrender_bg(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->line = 250;
    ppu->dot = 10;
    ppu->pxpl.mux = 0x4;
    ppu->pxpl.pal = 0x5;
    ppu->pxpl.px = 0x7;

    for (int i = 0; i < 9; ++i) {
        aldo_ppu_cycle(ppu);

        ct_assertequal(10 + i + 1, ppu->dot);
        ct_assertequal(0x4u, ppu->pxpl.mux);
        ct_assertequal(0x5u, ppu->pxpl.pal);
        ct_assertequal(0x7u, ppu->pxpl.px);
        ct_assertfalse(ppu->signal.vout);
    }
}

static void pixel_transparent_bg(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.bgs[0] = 0x1fff;
    ppu->pxpl.bgs[1] = 0x1fff;
    ppu->pxpl.ats[0] = 0xbf;
    ppu->pxpl.ats[1] = 0x7f;
    ppu->pxpl.mux = 0x0;
    ppu->pxpl.pal = 0x6;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x4u, ppu->pxpl.mux);
    ct_assertequal(0x0u, ppu->pxpl.pal);
    ct_assertequal(0x11u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x8u, ppu->pxpl.mux);
    ct_assertequal(0x0u, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xcu, ppu->pxpl.mux);
    ct_assertequal(0x0u, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xfu, ppu->pxpl.mux);
    ct_assertequal(0x0u, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xfu, ppu->pxpl.mux);
    ct_assertequal(0xfu, ppu->pxpl.pal);
    ct_assertequal(0x24u, ppu->pxpl.px);
}

static void pixel_disabled_bg(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.bgs[1] = ppu->pxpl.bgs[0] = 0xffff;
    ppu->pxpl.ats[1] = ppu->pxpl.ats[0] = 0xff;
    ppu->mask.b = false;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0xcu, ppu->pxpl.mux);
}

static void left_mask_bg(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->line = 10;
    ppu->dot = 2;
    ppu->pxpl.atl[1] = ppu->pxpl.atl[0] = true;
    ppu->pxpl.ats[1] = ppu->pxpl.ats[0] = ppu->pxpl.bg[1] = ppu->pxpl.bg[0] =
        0xff;
    ppu->pxpl.bgs[1] = ppu->pxpl.bgs[0] = 0xffff;
    ppu->mask.bm = false;

    for (int i = 0; i < 8; ++i) {
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
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);

    uint8_t bg0 = 0, bg1 = 0, at0 = 0, at1 = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        ppu->dot = 70;
        ppu->pxpl.bgs[0] = 0xcd00;
        ppu->pxpl.bgs[1] = 0xa800;
        ppu->pxpl.ats[0] = 0x73;
        ppu->pxpl.ats[1] = 0x41;
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
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.mux = 0xf;
    ppu->mask.s = ppu->mask.b = false;
    ppu->v = 0x20ff;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x0u, ppu->pxpl.pal);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x24u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

static void rendering_disabled_explicit_bg_palette(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    ppu->dot = 66;
    ppu->pxpl.mux = 0xf;
    ppu->mask.s = ppu->mask.b = false;
    ppu->v = 0x3f09;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x9u, ppu->pxpl.pal);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0x5u, ppu->pxpl.px);
    ct_asserttrue(ppu->signal.vout);
}

static void rendering_disabled_unused_bg_palette(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
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
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
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
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
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

struct ct_testsuite ppu_render_tests(void)
{
    static const struct ct_testcase tests[] = {
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

        ct_maketest(course_x_wraparound),
        ct_maketest(fine_y_wraparound),
        ct_maketest(course_y_wraparound),
        ct_maketest(course_y_overflow),

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

    return ct_makesuite_setup_teardown(tests, ppu_render_setup, ppu_teardown);
}
