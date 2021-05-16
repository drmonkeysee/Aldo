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

//
// Test List
//

struct ct_testsuite bus_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(new_bus),
    };

    return ct_makesuite_setup_teardown(tests, setup, teardown);
}
