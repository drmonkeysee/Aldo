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

static bool test_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    uint8_t *mem = ctx;
    if (addr > 3) return false;
    *d = mem[addr];
    return true;
}

static bool test_high_read(void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    uint8_t *mem = ctx;
    *d = mem[addr & 0x1f];
    return true;
}

static bool test_write(void *ctx, uint16_t addr, uint8_t d)
{
    uint8_t *mem = ctx;
    if (addr > 3) return false;
    mem[addr] = d;
    return true;
}

static bool test_high_write(void *ctx, uint16_t addr, uint8_t d)
{
    uint8_t *mem = ctx;
    mem[addr & 0x1f] = d;
    return true;
}

static void setup(void **ctx)
{
    // 6-bit address space (64 bytes), 3 partitions,
    // [$00 - $0F, $10 - $1F, $20 - $3F]
    *ctx = bus_new(6, 3, 0x10, 0x20);
}

static void teardown(void **ctx)
{
    bus_free(*ctx);
}

static void new_bus(void *ctx)
{
    bus *const b = ctx;

    ct_assertequal(3u, bus_count(b));
    ct_assertequal(0x3fu, bus_maxaddr(b));
    ct_assertequal(0x0, bus_pstart(b, 0));
    ct_assertequal(0x10, bus_pstart(b, 1));
    ct_assertequal(0x20, bus_pstart(b, 2));
    ct_assertequal(EMUBUS_PARTITION_RANGE, bus_pstart(b, 3));
}

static void empty_device(void *ctx)
{
    bus *const b = ctx;

    uint8_t d = 0xa;

    ct_assertfalse(bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);
    ct_assertfalse(bus_write(b, 0x0, d));
}

static void read_device(void *ctx)
{
    bus *const b = ctx;
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    const struct busdevice bd = {
        .read = test_read,
        .ctx = mem,
    };

    ct_asserttrue(bus_set(b, 0x0, bd));

    uint8_t d = 0xff;
    ct_asserttrue(bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(bus_read(b, 0x3, &d));
    ct_assertequal(0xdu, d);

    ct_assertfalse(bus_read(b, 0x4, &d));
    ct_assertequal(0xdu, d);

    ct_assertfalse(bus_read(b, 0x10, &d));
    ct_assertequal(0xdu, d);
}

static void read_device_at_high_partition(void *ctx)
{
    bus *const b = ctx;
    uint8_t memlow[] = {0x9, 0x8, 0x7, 0x6},
            memhigh[] = {0xa, 0xb, [30] = 0xc, [31] = 0xd};
    struct busdevice bd = {
        .read = test_read,
        .ctx = memlow,
    };

    ct_asserttrue(bus_set(b, 0x0, bd));

    bd = (struct busdevice){.read = test_high_read, .ctx = memhigh};
    ct_asserttrue(bus_set(b, 0x20, bd));

    uint8_t d = 0xff;
    ct_asserttrue(bus_read(b, 0x20, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(bus_read(b, 0x3f, &d));
    ct_assertequal(0xdu, d);

    ct_assertfalse(bus_read(b, 0x1a, &d));
    ct_assertequal(0xdu, d);

    ct_asserttrue(bus_read(b, 0x40, &d));
    ct_assertequal(0x9u, d);
}

static void write_device(void *ctx)
{
    bus *const b = ctx;
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff};
    const struct busdevice bd = {
        .write = test_write,
        .ctx = mem,
    };

    ct_asserttrue(bus_set(b, 0x0, bd));

    ct_asserttrue(bus_write(b, 0x0, 0xa));
    ct_assertequal(0xau, mem[0]);

    ct_asserttrue(bus_write(b, 0x3, 0xd));
    ct_assertequal(0xdu, mem[3]);

    ct_assertfalse(bus_write(b, 0x4, 0xe));

    ct_assertfalse(bus_write(b, 0x10, 0xc));
}

static void write_device_at_high_partition(void *ctx)
{
    bus *const b = ctx;
    uint8_t memlow[] = {0xff, 0xff, 0xff, 0xff},
            memhigh[] = {0xff, 0xff, [30] = 0xff, [31] = 0xff};
    struct busdevice bd = {
        .write = test_write,
        .ctx = memlow,
    };

    ct_asserttrue(bus_set(b, 0x0, bd));

    bd = (struct busdevice){.write = test_high_write, .ctx = memhigh};
    ct_asserttrue(bus_set(b, 0x20, bd));

    ct_asserttrue(bus_write(b, 0x20, 0xa));
    ct_assertequal(0xau, memhigh[0]);

    ct_asserttrue(bus_write(b, 0x3f, 0xd));
    ct_assertequal(0xdu, memhigh[31]);

    ct_assertfalse(bus_write(b, 0x1a, 0xe));

    ct_asserttrue(bus_write(b, 0x40, 0xc));
    ct_assertequal(0xcu, memlow[0]);
}

static void set_device_within_partition_bounds(void *ctx)
{
    bus *const b = ctx;
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff};
    const struct busdevice bd = {
        .read = test_high_read,
        .ctx = mem,
    };
    uint8_t d;

    ct_asserttrue(bus_set(b, 0x30, bd));
    ct_assertequal(0x20, bus_pstart(b, 2));
    ct_assertequal(EMUBUS_PARTITION_RANGE, bus_pstart(b, 3));
    ct_asserttrue(bus_read(b, 0x20, &d));
    ct_assertequal(0xffu, d);
}

static void set_device_too_high(void *ctx)
{
    bus *const b = ctx;
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff};
    const struct busdevice bd = {
        .read = test_read,
        .ctx = mem,
    };
    uint8_t d;

    ct_assertfalse(bus_set(b, 0x40, bd));
    ct_assertequal(0x20, bus_pstart(b, 2));
    ct_assertequal(EMUBUS_PARTITION_RANGE, bus_pstart(b, 3));
    ct_assertfalse(bus_read(b, 0x40, &d));
}

static void device_swap(void *ctx)
{
    bus *const b = ctx;
    uint8_t mem1[] = {0xff, 0xff, 0xff, 0xff},
            mem2[] = {0xff, 0xff, 0xff, 0xff};
    struct busdevice bd1 = {
        .write = test_write,
        .ctx = mem1,
    }, bd2 = {
        .write = test_write,
        .ctx = mem2,
    };

    ct_asserttrue(bus_set(b, 0x0, bd1));
    ct_asserttrue(bus_write(b, 0x0, 0xa));
    ct_assertequal(0xau, mem1[0]);
    ct_assertequal(0xffu, mem2[0]);

    struct busdevice bd3;
    ct_asserttrue(bus_swap(b, 0x0, bd2, &bd3));
    ct_asserttrue(bus_write(b, 0x0, 0xb));
    ct_assertequal(0xau, mem1[0]);
    ct_assertequal(0xbu, mem2[0]);

    ct_assertsame(mem1, bd3.ctx);
    ct_assertnull(bd3.read);
    ct_assertsame(test_write, bd3.write);
}

//
// Test List
//

struct ct_testsuite bus_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(new_bus),
        ct_maketest(empty_device),

        ct_maketest(read_device),
        ct_maketest(read_device_at_high_partition),

        ct_maketest(write_device),
        ct_maketest(write_device_at_high_partition),

        ct_maketest(set_device_within_partition_bounds),
        ct_maketest(set_device_too_high),

        ct_maketest(device_swap),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
