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
        // TODO: assume horizontal mirroring for now (Donkey Kong setting)
        size_t select = addr < 0x2800 ? 0 : 1;
        *d = (addr & 0xff) < 0xc0
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
// MARK: - Tests
//

static void nametable_fetch(void *ctx)
{
    struct aldo_rp2c02 *ppu = ppt_get_ppu(ctx);
    NameTables[0][5] = 0x11;
    ppu->v = 0x5;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(1u, ppu->dot);
    ct_assertequal(0u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(2u, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->nt);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(3u, ppu->dot);
    ct_assertequal(0x2005u, ppu->vaddrbus);
    ct_assertequal(0x11u, ppu->vdatabus);
    ct_assertequal(0x11u, ppu->nt);
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

    ct_assertequal(0u, ppu->line);
    ct_assertequal(4u, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->at);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(5u, ppu->dot);
    ct_assertequal(0x23c1u, ppu->vaddrbus);
    ct_assertequal(0x22u, ppu->vdatabus);
    ct_assertequal(0x22u, ppu->at);
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
    ppu->nt = 0x11;

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(6u, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0u, ppu->vdatabus);
    ct_assertequal(0u, ppu->bg[0]);
    ct_assertequal(0u, ppu->bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(7u, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0110u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->bg[0]);
    ct_assertequal(0u, ppu->bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(8u, ppu->dot);
    ct_assertequal(0x5u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x33u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->bg[0]);
    ct_assertequal(0u, ppu->bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_asserttrue(ppu->signal.ale);
    ct_asserttrue(ppu->signal.rd);

    aldo_ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(9u, ppu->dot);
    ct_assertequal(0x6u, ppu->v);
    ct_assertequal(0x0118u, ppu->vaddrbus);
    ct_assertequal(0x44u, ppu->vdatabus);
    ct_assertequal(0x33u, ppu->bg[0]);
    ct_assertequal(0x44u, ppu->bg[1]);
    ct_assertequal(0u, ppu->rbuf);
    ct_assertfalse(ppu->signal.ale);
    ct_assertfalse(ppu->signal.rd);
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
    };

    return ct_makesuite_setup_teardown(tests, ppu_render_setup, ppu_teardown);
}
