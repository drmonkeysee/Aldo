//
//  ppu.c
//  Tests
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "bus.h"
#include "bytes.h"
#include "ciny.h"
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

    ct_assertequal(0u, ppu->regd);
    ct_assertfalse(ppu->odd);
    ct_assertfalse(ppu->w);
}

//
// Test List
//

struct ct_testsuite ppu_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_ppu),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
