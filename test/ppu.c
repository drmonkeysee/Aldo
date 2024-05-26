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

static uint8_t VRam[256] = {0};

struct test_context {
    struct rp2c02 ppu;
    bus *mbus;
};

static void setup(void **ctx)
{
    struct test_context *const c = malloc(sizeof *c);
    // NOTE: enough main bus to map $2000 - $3FFF for ppu registers
    c->mbus = bus_new(14, 2, MEMBLOCK_8KB);
    ppu_connect(&c->ppu, VRam, c->mbus);
    ppu_powerup(&c->ppu);
    *ctx = c;
}

static void teardown(void **ctx)
{
    struct test_context *const c = (struct test_context *)*ctx;
    ppu_disconnect(&c->ppu);
    bus_free(c->mbus);
    free(c);
}

static void powerup_initializes_ppu(void *ctx)
{
    const struct rp2c02 *const ppu = get_ppu(ctx);

    ct_assertequal(CSGS_PENDING, (int)ppu->res);
    ct_assertequal(0u, ppu->signal.reg);
    ct_assertequal(0u, ppu->addr);
    ct_assertequal(0u, ppu->oamaddr);
    ct_asserttrue(ppu->signal.intr);
    ct_asserttrue(ppu->signal.res);
    ct_assertfalse(ppu->status.s);
}

static void trace_pixel_no_adjustment(void *ctx)
{
    const struct rp2c02 *const ppu = get_ppu(ctx);

    const struct ppu_coord pixel = ppu_pixel_trace(ppu, 0);

    ct_assertequal(0, pixel.dot);
    ct_assertequal(0, pixel.line);
}

static void trace_pixel_zero_with_adjustment(void *ctx)
{
    const struct rp2c02 *const ppu = get_ppu(ctx);

    const struct ppu_coord pixel = ppu_pixel_trace(ppu, -1);

    ct_assertequal(338, pixel.dot);
    ct_assertequal(261, pixel.line);
}

static void trace_pixel(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->dot = 223;
    ppu->line = 120;

    const struct ppu_coord pixel = ppu_pixel_trace(ppu, -1);

    ct_assertequal(220, pixel.dot);
    ct_assertequal(120, pixel.line);
}

static void trace_pixel_at_one_cycle(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->dot = 3;

    const struct ppu_coord pixel = ppu_pixel_trace(ppu, -1);

    ct_assertequal(0, pixel.dot);
    ct_assertequal(0, pixel.line);
}

static void trace_pixel_at_line_boundary(void *ctx)
{
    struct rp2c02 *const ppu = get_ppu(ctx);
    ppu->dot = 1;
    ppu->line = 120;

    const struct ppu_coord pixel = ppu_pixel_trace(ppu, -1);

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
        ct_maketest(trace_pixel_no_adjustment),
        ct_maketest(trace_pixel_zero_with_adjustment),
        ct_maketest(trace_pixel),
        ct_maketest(trace_pixel_at_one_cycle),
        ct_maketest(trace_pixel_at_line_boundary),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
