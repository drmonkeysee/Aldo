//
//  bus.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 5/15/21.
//

#include "bus.h"
#include "bytes.h"
#include "ciny.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define get_bus(ctx) ((struct test_context *)(ctx))->b

struct test_context {
    aldo_bus *b;
};

static bool test_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    const uint8_t *mem = ctx;
    if (addr > 3) return false;
    *d = mem[addr];
    return true;
}

static bool test_high_read(void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    const uint8_t *mem = ctx;
    *d = mem[addr & 0x1f];
    return true;
}

static bool test_highest_read(void *restrict ctx, uint16_t addr,
                              uint8_t *restrict d)
{
    const uint8_t *mem = ctx;
    *d = mem[addr % 4];
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

static bool test_highest_write(void *ctx, uint16_t addr, uint8_t d)
{
    uint8_t *mem = ctx;
    mem[addr % 4] = d;
    return true;
}

static size_t test_copy(const void *restrict ctx, uint16_t addr, size_t count,
                        uint8_t dest[restrict count])
{
    if (addr >= 4) return 0;
    const uint8_t *mem = ctx;
    size_t maxlength = 4 - addr;
    memcpy(dest, mem + addr, maxlength * sizeof *dest);
    return maxlength;
}

static void setup(void **ctx)
{
    struct test_context *c = malloc(sizeof *c);
    // 6-bit address space (64 bytes), 3 partitions,
    // [$00 - $0F, $10 - $1F, $20 - $3F]
    c->b = aldo_bus_new(6, 3, 0x10, 0x20);
    *ctx = c;
}

static void teardown(void **ctx)
{
    aldo_bus_free(((struct test_context *)*ctx)->b);
    free(*ctx);
}

static void empty_device(void *ctx)
{
    aldo_bus *b = get_bus(ctx);

    uint8_t d = 0xa;

    ct_assertfalse(aldo_bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);
    ct_assertfalse(aldo_bus_write(b, 0x0, d));
}

static void read_device(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .read = test_read,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t d = 0xff;
    ct_asserttrue(aldo_bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(aldo_bus_read(b, 0x3, &d));
    ct_assertequal(0xdu, d);

    ct_assertfalse(aldo_bus_read(b, 0x4, &d));
    ct_assertequal(0xdu, d);

    ct_assertfalse(aldo_bus_read(b, 0x10, &d));
    ct_assertequal(0xdu, d);
}

static void read_device_at_high_partition(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t memlow[] = {0x9, 0x8, 0x7, 0x6},
            memhigh[] = {0xa, 0xb, [30] = 0xc, [31] = 0xd};
    struct aldo_busdevice bd = {
        .read = test_read,
        .ctx = memlow,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    bd = (struct aldo_busdevice){.read = test_high_read, .ctx = memhigh};
    ct_asserttrue(aldo_bus_set(b, 0x20, bd));

    uint8_t d = 0xff;
    ct_asserttrue(aldo_bus_read(b, 0x20, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(aldo_bus_read(b, 0x3f, &d));
    ct_assertequal(0xdu, d);

    ct_assertfalse(aldo_bus_read(b, 0x1a, &d));
    ct_assertequal(0xdu, d);

    ct_assertfalse(aldo_bus_read(b, 0x40, &d));
    ct_assertequal(0xdu, d);
}

static void write_device(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff};
    struct aldo_busdevice bd = {
        .write = test_write,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    ct_asserttrue(aldo_bus_write(b, 0x0, 0xa));
    ct_assertequal(0xau, mem[0]);

    ct_asserttrue(aldo_bus_write(b, 0x3, 0xd));
    ct_assertequal(0xdu, mem[3]);

    ct_assertfalse(aldo_bus_write(b, 0x4, 0xe));

    ct_assertfalse(aldo_bus_write(b, 0x10, 0xc));
}

static void write_device_at_high_partition(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t memlow[] = {0xff, 0xff, 0xff, 0xff},
            memhigh[] = {0xff, 0xff, [30] = 0xff, [31] = 0xff};
    struct aldo_busdevice bd = {
        .write = test_write,
        .ctx = memlow,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    bd = (struct aldo_busdevice){.write = test_high_write, .ctx = memhigh};
    ct_asserttrue(aldo_bus_set(b, 0x20, bd));

    ct_asserttrue(aldo_bus_write(b, 0x20, 0xa));
    ct_assertequal(0xau, memhigh[0]);

    ct_asserttrue(aldo_bus_write(b, 0x3f, 0xd));
    ct_assertequal(0xdu, memhigh[31]);

    ct_assertfalse(aldo_bus_write(b, 0x1a, 0xe));

    ct_assertfalse(aldo_bus_write(b, 0x40, 0xe));
    ct_assertequal(0xffu, memlow[0]);
}

static void set_device_within_partition_bounds(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff};
    struct aldo_busdevice bd = {
        .read = test_high_read,
        .ctx = mem,
    };
    uint8_t d;

    ct_asserttrue(aldo_bus_set(b, 0x30, bd));
    ct_asserttrue(aldo_bus_read(b, 0x20, &d));
    ct_assertequal(0xffu, d);
}

static void set_device_too_high(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff};
    struct aldo_busdevice bd = {
        .read = test_read,
        .ctx = mem,
    };
    uint8_t d;

    ct_assertfalse(aldo_bus_set(b, 0x40, bd));
    ct_assertfalse(aldo_bus_read(b, 0x40, &d));
}

static void device_swap(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem1[] = {0xff, 0xff, 0xff, 0xff},
            mem2[] = {0xff, 0xff, 0xff, 0xff};
    struct aldo_busdevice bd1 = {
        .write = test_write,
        .ctx = mem1,
    }, bd2 = {
        .write = test_write,
        .ctx = mem2,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd1));
    ct_asserttrue(aldo_bus_write(b, 0x0, 0xa));
    ct_assertequal(0xau, mem1[0]);
    ct_assertequal(0xffu, mem2[0]);

    struct aldo_busdevice bd3;
    ct_asserttrue(aldo_bus_swap(b, 0x0, bd2, &bd3));
    ct_asserttrue(aldo_bus_write(b, 0x0, 0xb));
    ct_assertequal(0xau, mem1[0]);
    ct_assertequal(0xbu, mem2[0]);

    ct_assertsame(mem1, bd3.ctx);
    ct_assertnull((void *)bd3.read);
    ct_assertsame((void *)test_write, (void *)bd3.write);
}

static void device_clear(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff};
    struct aldo_busdevice bd1 = {
        .read = test_read,
        .write = test_write,
        .ctx = mem,
    };
    uint8_t d = 0xff;

    ct_asserttrue(aldo_bus_set(b, 0x0, bd1));
    ct_asserttrue(aldo_bus_write(b, 0x0, 0xa));
    ct_assertequal(0xau, mem[0]);
    ct_asserttrue(aldo_bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);

    d = 0xff;
    ct_asserttrue(aldo_bus_clear(b, 0x0));
    ct_assertfalse(aldo_bus_write(b, 0x0, 0xb));
    ct_assertfalse(aldo_bus_read(b, 0x0, &d));
    ct_assertequal(0xffu, d);
}

static void smallest_bus(void *ctx)
{
    struct test_context *c = ctx;
    aldo_bus_free(c->b);
    aldo_bus *b = c->b = aldo_bus_new(1, 1);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .read = test_read,
        .write = test_write,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t d = 0xff;
    ct_asserttrue(aldo_bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(aldo_bus_read(b, 0x1, &d));
    ct_assertequal(0xbu, d);

    ct_assertfalse(aldo_bus_read(b, 0x2, &d));
    ct_assertequal(0xbu, d);

    ct_asserttrue(aldo_bus_write(b, 0x0, 0x9));
    ct_assertequal(0x9u, mem[0]);

    ct_asserttrue(aldo_bus_write(b, 0x1, 0x8));
    ct_assertequal(0x8u, mem[1]);

    ct_assertfalse(aldo_bus_write(b, 0x2, 0x7));
}

static void largest_bus(void *ctx)
{
    struct test_context *c = ctx;
    aldo_bus_free(c->b);
    aldo_bus *b = c->b = aldo_bus_new(ALDO_BITWIDTH_64KB, 2, 0x8000);
    uint8_t memlow[] = {0xa, 0xb, 0xc, 0xd},
            memhigh[] = {0x9, 0x8, 0x7, 0x6};
    struct aldo_busdevice bd = {
        .read = test_read,
        .write = test_write,
        .ctx = memlow,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    bd = (struct aldo_busdevice){
        .read = test_highest_read,
        .write = test_highest_write,
        .ctx = memhigh,
    };
    ct_asserttrue(aldo_bus_set(b, 0x8000, bd));

    uint8_t d = 0xff;
    ct_asserttrue(aldo_bus_read(b, 0xffff, &d));
    ct_assertequal(0x6u, d);

    ct_asserttrue(aldo_bus_read(b, 0x0, &d));
    ct_assertequal(0xau, d);

    ct_asserttrue(aldo_bus_write(b, 0xffff, 0x5));
    ct_assertequal(0x5u, memhigh[3]);

    ct_asserttrue(aldo_bus_write(b, 0x0, 0x4));
    ct_assertequal(0x4u, memlow[0]);
}

static void copy(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .copy = test_copy,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t dest[4];
    size_t count = aldo_bus_copy(b, 0x0, sizeof dest / sizeof dest[0], dest);
    ct_assertequal(4u, count);
    ct_assertequal(mem[0], dest[0]);
    ct_assertequal(mem[1], dest[1]);
    ct_assertequal(mem[2], dest[2]);
    ct_assertequal(mem[3], dest[3]);
}

static void copy_partial_bank(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .copy = test_copy,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t dest[4];
    size_t count = aldo_bus_copy(b, 0x1, sizeof dest / sizeof dest[0], dest);
    ct_assertequal(3u, count);
    ct_assertequal(mem[1], dest[0]);
    ct_assertequal(mem[2], dest[1]);
    ct_assertequal(mem[3], dest[2]);
}

static void copy_end_of_bank(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .copy = test_copy,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t dest[4];
    size_t count = aldo_bus_copy(b, 0x3, sizeof dest / sizeof dest[0], dest);
    ct_assertequal(1u, count);
    ct_assertequal(mem[3], dest[0]);
}

static void copy_past_bank(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .copy = test_copy,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t dest[] = {0xff, 0xff, 0xff, 0xff};
    size_t count = aldo_bus_copy(b, 0x4, sizeof dest / sizeof dest[0], dest);
    ct_assertequal(0u, count);
    ct_assertequal(0xffu, dest[0]);
}

static void copy_wrong_addr(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .copy = test_copy,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t dest[] = {0xff, 0xff, 0xff, 0xff};
    size_t count = aldo_bus_copy(b, 0x40, sizeof dest / sizeof dest[0], dest);
    ct_assertequal(0u, count);
    ct_assertequal(0xffu, dest[0]);
}

static void copy_zero_count(void *ctx)
{
    aldo_bus *b = get_bus(ctx);
    uint8_t mem[] = {0xa, 0xb, 0xc, 0xd};
    struct aldo_busdevice bd = {
        .copy = test_copy,
        .ctx = mem,
    };

    ct_asserttrue(aldo_bus_set(b, 0x0, bd));

    uint8_t dest[] = {0xff, 0xff, 0xff, 0xff};
    size_t count = aldo_bus_copy(b, 0x0, 0, dest);
    ct_assertequal(0u, count);
    ct_assertequal(0xffu, dest[0]);
}

//
// MARK: - Test List
//

struct ct_testsuite bus_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(empty_device),
        ct_maketest(read_device),
        ct_maketest(read_device_at_high_partition),
        ct_maketest(write_device),
        ct_maketest(write_device_at_high_partition),
        ct_maketest(set_device_within_partition_bounds),
        ct_maketest(set_device_too_high),
        ct_maketest(device_swap),
        ct_maketest(device_clear),
        ct_maketest(smallest_bus),
        ct_maketest(largest_bus),
        ct_maketest(copy),
        ct_maketest(copy_partial_bank),
        ct_maketest(copy_end_of_bank),
        ct_maketest(copy_past_bank),
        ct_maketest(copy_wrong_addr),
        ct_maketest(copy_zero_count),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
