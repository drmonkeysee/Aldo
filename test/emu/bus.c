//
//  bus.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 5/15/21.
//

#include "ciny.h"
#include "emu/bus.h"

#include <stdbool.h>
#include <stdint.h>

static void setup(void **ctx)
{
    // 6-bit address space (64 bytes), 4-bit pages (16 bytes), 4 pages total
    *ctx = bus_new(6, 4);
}

static void teardown(void **ctx)
{
    bus_free(*ctx);
}

static void new_bank(void *ctx)
{
    bus *const b = ctx;

    ct_assertequal(0u, bus_size(b));
    ct_assertequal(0u, bus_capacity(b));
    ct_assertequal(8u, bus_pagecount(b));
    ct_assertequal(0x3fu, bus_maxaddr(b));
}

static void read_simple_bank(void *ctx)
{
    bus *const b = ctx;

    uint8_t mem[16] = {0xa, 0xb, 0xc, [15] = 0xd};

    ct_asserttrue(bus_add(b, sizeof mem / sizeof mem[0], mem, 0,
                          (sizeof mem / sizeof mem[0]) - 1, MMODE_READ));
    ct_assertequal(1u, bus_size(b));
    ct_assertequal(1u, bus_capacity(b));

    uint8_t d;
    ct_asserttrue(bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(bus_read(b, 0xf, &d));
    ct_assertequal(0xdu, d);

    ct_asserttrue(bus_read(b, 0x2, &d));
    ct_assertequal(0xcu, d);

    // NOTE: address outside bank range
    ct_assertfalse(bus_read(b, 0x10, &d));
    ct_assertequal(0xcu, d);

    // NOTE: address outside map range
    ct_assertfalse(bus_read(b, 0x40, &d));
    ct_assertequal(0xcu, d);
}

static void read_small_bank(void *ctx)
{
    bus *const b = ctx;

    uint8_t mem[16] = {
        0xe, 0xe, 0xe, 0xa, 0xb, 0xc, [12] = 0xd, 0xf, [15] = 0xf,
    };

    ct_asserttrue(bus_add(b, 8, mem, 0x3, 0xb, MMODE_READ));
    ct_assertequal(1u, bus_size(b));
    ct_assertequal(1u, bus_capacity(b));

    uint8_t d;
    ct_asserttrue(bus_read(b, 0x3, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(bus_read(b, 0xb, &d));
    ct_assertequal(0xdu, d);

    ct_asserttrue(bus_read(b, 0x5, &d));
    ct_assertequal(0xcu, d);

    ct_assertfalse(bus_read(b, 0x0, &d));
    ct_assertequal(0xcu, d);

    ct_assertfalse(bus_read(b, 0xc, &d));
    ct_assertequal(0xcu, d);

    // NOTE: address outside bank range
    ct_assertfalse(bus_read(b, 0x10, &d));
    ct_assertequal(0xcu, d);

    // NOTE: address outside map range
    ct_assertfalse(bus_read(b, 0x40, &d));
    ct_assertequal(0xcu, d);
}

static void write_simple_bank(void *ctx)
{
    bus *const b = ctx;

    uint8_t mem[16] = {0xff, 0xff, 0xff, [15] = 0xff};

    ct_asserttrue(bus_add(b, sizeof mem / sizeof mem[0], mem, 0,
                          (sizeof mem / sizeof mem[0]) - 1, MMODE_WRITE));
    ct_assertequal(1u, bus_size(b));
    ct_assertequal(1u, bus_capacity(b));

    ct_asserttrue(bus_write(b, 0x0, 0xa));
    ct_assertequal(0xau, mem[0]);

    ct_asserttrue(bus_write(b, 0xf, 0xb));
    ct_assertequal(0xbu, mem[15]);

    ct_asserttrue(bus_write(b, 0x2, 0xc));
    ct_assertequal(0xcu, mem[2]);

    // NOTE: address outside bank range
    ct_assertfalse(bus_write(b, 0x10, 0xd));

    // NOTE: address outside map range
    ct_assertfalse(bus_write(b, 0x40, 0xd));
}

static void write_small_bank(void *ctx)
{
    bus *const b = ctx;

    uint8_t mem[16] = {
        0xee, 0xee, 0xee, 0xff, 0xff, 0xff, [12] = 0xff, 0xee, [15] = 0xee,
    };

    ct_asserttrue(bus_add(b, 8, mem, 0x3, 0xb, MMODE_WRITE));
    ct_assertequal(1u, bus_size(b));
    ct_assertequal(1u, bus_capacity(b));

    ct_asserttrue(bus_write(b, 0x3, 0xa));
    ct_assertequal(0xau, mem[3]);

    ct_asserttrue(bus_write(b, 0xb, 0xb));
    ct_assertequal(0xbu, mem[12]);

    ct_asserttrue(bus_write(b, 0x5, 0xc));
    ct_assertequal(0xcu, mem[5]);

    ct_assertfalse(bus_write(b, 0x0, 0xd));
    ct_assertequal(0xeeu, mem[0]);

    ct_assertfalse(bus_write(b, 0xc, 0xd));
    ct_assertequal(0xeeu, mem[0]);

    // NOTE: address outside bank range
    ct_assertfalse(bus_write(b, 0x10, 0xd));

    // NOTE: address outside map range
    ct_assertfalse(bus_write(b, 0x40, 0xd));
}

//
// Test List
//

struct ct_testsuite bus_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(new_bank),
        ct_maketest(read_simple_bank),
        ct_maketest(read_small_bank),
        ct_maketest(write_simple_bank),
        ct_maketest(write_small_bank),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
