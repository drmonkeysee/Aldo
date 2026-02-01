//
//  bytes.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 6/3/21.
//

#include "bytes.h"
#include "ciny.h"

#include <stddef.h>
#include <stdint.h>

static void shuffle_zeros(void *ctx)
{
    uint8_t lo = 0x0,
            hi = 0x0;

    auto s = aldo_byteshuffle(lo, hi);

    ct_assertequal(0u, s);
}

static void shuffle_ones(void *ctx)
{
    uint8_t lo = 0xff,
            hi = 0xff;

    auto s = aldo_byteshuffle(lo, hi);

    ct_assertequal(0xffffu, s);
}

static void shuffle_low_ones(void *ctx)
{
    uint8_t lo = 0xff,
            hi = 0x0;

    auto s = aldo_byteshuffle(lo, hi);

    ct_assertequal(0x5555u, s);
}

static void shuffle_high_ones(void *ctx)
{
    uint8_t lo = 0x0,
            hi = 0xff;

    auto s = aldo_byteshuffle(lo, hi);

    ct_assertequal(0xaaaau, s);
}

static void shuffle_mixed(void *ctx)
{
    uint8_t lo = 0x55,
            hi = 0xaa;

    auto s = aldo_byteshuffle(lo, hi);

    ct_assertequal(0x9999u, s);
}

static void shuffle_flip_mixed(void *ctx)
{
    uint8_t lo = 0xaa,
            hi = 0x55;

    auto s = aldo_byteshuffle(lo, hi);

    ct_assertequal(0x6666u, s);
}

static void shuffle_arbitrary(void *ctx)
{
    uint8_t lo = 0x1c,
            hi = 0x74;

    auto s = aldo_byteshuffle(lo, hi);

    ct_assertequal(0x2b70u, s);
}

static void bank_copy_zero_count(void *ctx)
{
    uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest = 0x11;

    auto result = aldo_bytecopy_bank(bank, ALDO_BITWIDTH_1KB, 0x0, 0, &dest);

    ct_assertequal(0u, result);
    ct_assertequal(0x11u, dest);
}

static void bank_copy(void *ctx)
{
    uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest[5];

    auto result = aldo_bytecopy_bank(bank, ALDO_BITWIDTH_1KB, 0x0,
                                     sizeof dest / sizeof dest[0], dest);

    ct_assertequal(5u, result);
    ct_assertequal(0xaau, dest[0]);
    ct_assertequal(0x99u, dest[1]);
    ct_assertequal(0x88u, dest[2]);
    ct_assertequal(0x77u, dest[3]);
    ct_assertequal(0x66u, dest[4]);
}

static void bank_copy_end_of_bank(void *ctx)
{
    uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest[5] = {[1] = 0x11};

    auto result = aldo_bytecopy_bank(bank, ALDO_BITWIDTH_1KB, 0x3ff,
                                     sizeof dest / sizeof dest[0], dest);

    ct_assertequal(1u, result);
    ct_assertequal(0xffu, dest[0]);
    ct_assertequal(0x11u, dest[1]);
}

static void bank_copy_fit_end_of_bank(void *ctx)
{
    uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xee,
    };
    uint8_t dest[] = {0x11, 0x11, 0x11, 0x11, 0x11};

    auto result = aldo_bytecopy_bank(bank, ALDO_BITWIDTH_1KB, 0x3fb,
                                     sizeof dest / sizeof dest[0], dest);

    ct_assertequal(5u, result);
    ct_assertequal(0u, dest[0]);
    ct_assertequal(0u, dest[1]);
    ct_assertequal(0xffu, dest[2]);
    ct_assertequal(0xffu, dest[3]);
    ct_assertequal(0xeeu, dest[4]);
}

static void bank_copy_address_beyond_range(void *ctx)
{
    uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest[5];

    auto result = aldo_bytecopy_bank(bank, ALDO_BITWIDTH_1KB, 0x401,
                                     sizeof dest / sizeof dest[0], dest);

    ct_assertequal(5u, result);
    ct_assertequal(0x99u, dest[0]);
    ct_assertequal(0x88u, dest[1]);
    ct_assertequal(0x77u, dest[2]);
    ct_assertequal(0x66u, dest[3]);
    ct_assertequal(0x55u, dest[4]);
}

//
// MARK: - Test List
//

struct ct_testsuite bytes_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(shuffle_zeros),
        ct_maketest(shuffle_ones),
        ct_maketest(shuffle_low_ones),
        ct_maketest(shuffle_high_ones),
        ct_maketest(shuffle_mixed),
        ct_maketest(shuffle_flip_mixed),
        ct_maketest(shuffle_arbitrary),

        ct_maketest(bank_copy_zero_count),
        ct_maketest(bank_copy),
        ct_maketest(bank_copy_end_of_bank),
        ct_maketest(bank_copy_fit_end_of_bank),
        ct_maketest(bank_copy_address_beyond_range),
    };

    return ct_makesuite(tests);
}
