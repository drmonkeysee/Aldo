//
//  memorymap.c
//  AldoTests
//
//  Created by Brandon Stansbury on 5/9/21.
//

#include "ciny.h"
#include "emu/memorymap.h"

#include <stdbool.h>
#include <stdint.h>

static void setup(void **ctx)
{
    *ctx = memmap_new();
}

static void teardown(void **ctx)
{
    memmap_free(*ctx);
}

static void read_simple_bank(void *ctx)
{
    memmap *const m = ctx;

    uint8_t mem[16] = {0xa, 0xb, 0xc, [15] = 0xd};
    const struct memorybank b = {
        .size = sizeof mem / sizeof mem[0],
        .mem = mem,
        .addrmax = (sizeof mem / sizeof mem[0]) - 1,
    };

    ct_asserttrue(memmap_add(m, &b));

    uint8_t d;
    ct_asserttrue(memmap_read(m, 0x0, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(memmap_read(m, 0xf, &d));
    ct_assertequal(0xdu, d);

    ct_asserttrue(memmap_read(m, 0x2, &d));
    ct_assertequal(0xcu, d);

    ct_assertfalse(memmap_read(m, 0x10, &d));
    ct_assertequal(0xcu, d);
}

static void read_small_bank(void *ctx)
{
    memmap *const m = ctx;

    uint8_t mem[16] = {
        0xe, 0xe, 0xe, 0xa, 0xb, 0xc, [12] = 0xd, 0xf, [15] = 0xf,
    };
    const struct memorybank b = {
        .size = 8,
        .mem = mem,
        .addrmin = 0x3,
        .addrmax = 0xb,
    };

    ct_asserttrue(memmap_add(m, &b));

    uint8_t d;
    ct_asserttrue(memmap_read(m, 0x3, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(memmap_read(m, 0xb, &d));
    ct_assertequal(0xdu, d);

    ct_asserttrue(memmap_read(m, 0x5, &d));
    ct_assertequal(0xcu, d);

    ct_assertfalse(memmap_read(m, 0x0, &d));
    ct_assertequal(0xcu, d);

    ct_assertfalse(memmap_read(m, 0xc, &d));
    ct_assertequal(0xcu, d);

    ct_assertfalse(memmap_read(m, 0x10, &d));
    ct_assertequal(0xcu, d);
}

static void write_simple_bank(void *ctx)
{
    memmap *const m = ctx;

    uint8_t mem[16] = {0xff, 0xff, 0xff, [15] = 0xff};
    const struct memorybank b = {
        .size = sizeof mem / sizeof mem[0],
        .mem = mem,
        .addrmax = (sizeof mem / sizeof mem[0]) - 1,
    };

    ct_asserttrue(memmap_add(m, &b));

    ct_asserttrue(memmap_write(m, 0x0, 0xa));
    ct_assertequal(0xau, mem[0]);

    ct_asserttrue(memmap_write(m, 0xf, 0xb));
    ct_assertequal(0xbu, mem[15]);

    ct_asserttrue(memmap_write(m, 0x2, 0xc));
    ct_assertequal(0xcu, mem[2]);

    ct_assertfalse(memmap_write(m, 0x10, 0xd));
}

static void write_small_bank(void *ctx)
{
    memmap *const m = ctx;

    uint8_t mem[16] = {
        0xee, 0xee, 0xee, 0xff, 0xff, 0xff, [12] = 0xff, 0xee, [15] = 0xee,
    };
    const struct memorybank b = {
        .size = 8,
        .mem = mem,
        .addrmin = 0x3,
        .addrmax = 0xb,
    };

    ct_asserttrue(memmap_add(m, &b));

    ct_asserttrue(memmap_write(m, 0x3, 0xa));
    ct_assertequal(0xau, mem[3]);

    ct_asserttrue(memmap_write(m, 0xb, 0xb));
    ct_assertequal(0xbu, mem[12]);

    ct_asserttrue(memmap_write(m, 0x5, 0xc));
    ct_assertequal(0xcu, mem[5]);

    ct_assertfalse(memmap_write(m, 0x0, 0xd));
    ct_assertequal(0xeeu, mem[0]);

    ct_assertfalse(memmap_write(m, 0xc, 0xd));
    ct_assertequal(0xeeu, mem[0]);

    ct_assertfalse(memmap_write(m, 0x10, 0xd));
}

//
// Test List
//

struct ct_testsuite memorymap_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(read_simple_bank),
        ct_maketest(read_small_bank),
        ct_maketest(write_simple_bank),
        ct_maketest(write_small_bank),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
