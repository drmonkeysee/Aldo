//
//  bytes.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 6/3/21.
//

#include "ciny.h"
#include "emu/bytes.h"

static void bank_copy_zero_count(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest = 0x11;

    const size_t result = bytecopy_bank(bank, 10, 0x0, 0, &dest);

    ct_assertequal(0u, result);
    ct_assertequal(0x11u, dest);
}

static void bank_copy(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest[5];

    const size_t result = bytecopy_bank(bank, 10, 0x0,
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
    const uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest[5] = {[1] = 0x11};

    const size_t result = bytecopy_bank(bank, 10, 0x3ff,
                                        sizeof dest / sizeof dest[0], dest);

    ct_assertequal(1u, result);
    ct_assertequal(0xffu, dest[0]);
    ct_assertequal(0x11u, dest[1]);
}

static void bank_copy_fit_end_of_bank(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xee,
    };
    uint8_t dest[] = {0x11, 0x11, 0x11, 0x11, 0x11};

    const size_t result = bytecopy_bank(bank, 10, 0x3fb,
                                        sizeof dest / sizeof dest[0], dest);

    ct_assertequal(5u, result);
    ct_assertequal(0x0u, dest[0]);
    ct_assertequal(0x0u, dest[1]);
    ct_assertequal(0xffu, dest[2]);
    ct_assertequal(0xffu, dest[3]);
    ct_assertequal(0xeeu, dest[4]);
}

static void bank_copy_masked_address(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest[5];

    const size_t result = bytecopy_bank(bank, 10, 0x401,
                                         sizeof dest / sizeof dest[0], dest);

    ct_assertequal(5u, result);
    ct_assertequal(0x99u, dest[0]);
    ct_assertequal(0x88u, dest[1]);
    ct_assertequal(0x77u, dest[2]);
    ct_assertequal(0x66u, dest[3]);
    ct_assertequal(0x55u, dest[4]);
}

static void bank_copymirrored_zero_count(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest = 0x11;

    const size_t result = bytecopy_bankmirrored(bank, 10, 0x0, 12, 0, &dest);

    ct_assertequal(0u, result);
    ct_assertequal(0x11u, dest);
}

static void bank_copymirrored_within_single_bank(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest[5];

    const size_t result = bytecopy_bankmirrored(bank, 10, 0x0, 12,
                                                sizeof dest / sizeof dest[0],
                                                dest);

    ct_assertequal(5u, result);
    ct_assertequal(0xaau, dest[0]);
    ct_assertequal(0x99u, dest[1]);
    ct_assertequal(0x88u, dest[2]);
    ct_assertequal(0x77u, dest[3]);
    ct_assertequal(0x66u, dest[4]);
}

static void bank_copymirrored_cross_bank_boundary(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55,
        [1021] = 0xff, [1022] = 0xee, [1023] = 0xdd,
    };
    uint8_t dest[5];

    const size_t result = bytecopy_bankmirrored(bank, 10, 0x3fe, 12,
                                                sizeof dest / sizeof dest[0],
                                                dest);

    ct_assertequal(5u, result);
    ct_assertequal(0xeeu, dest[0]);
    ct_assertequal(0xddu, dest[1]);
    ct_assertequal(0xaau, dest[2]);
    ct_assertequal(0x99u, dest[3]);
    ct_assertequal(0x88u, dest[4]);
}

static void bank_copymirrored_cross_middle_bank_boundary(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55,
        [1021] = 0xff, [1022] = 0xee, [1023] = 0xdd,
    };
    uint8_t dest[5];

    const size_t result = bytecopy_bankmirrored(bank, 10, 0xbfe, 12,
                                                sizeof dest / sizeof dest[0],
                                                dest);

    ct_assertequal(5u, result);
    ct_assertequal(0xeeu, dest[0]);
    ct_assertequal(0xddu, dest[1]);
    ct_assertequal(0xaau, dest[2]);
    ct_assertequal(0x99u, dest[3]);
    ct_assertequal(0x88u, dest[4]);
}

static void bank_copymirrored_end_of_address_space(void *ctx)
{
    const uint8_t bank[] = {
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55,
        [1021] = 0xff, [1022] = 0xee, [1023] = 0xdd,
    };
    uint8_t dest[5] = {0x11, 0x11, 0x11, 0x11, 0x11};

    const size_t result = bytecopy_bankmirrored(bank, 10, 0xfff, 12,
                                                sizeof dest / sizeof dest[0],
                                                dest);

    ct_assertequal(1u, result);
    ct_assertequal(0xddu, dest[0]);
    ct_assertequal(0x11u, dest[1]);
    ct_assertequal(0x11u, dest[2]);
    ct_assertequal(0x11u, dest[3]);
    ct_assertequal(0x11u, dest[4]);
}

//
// Test List
//

struct ct_testsuite bytes_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(bank_copy_zero_count),
        ct_maketest(bank_copy),
        ct_maketest(bank_copy_end_of_bank),
        ct_maketest(bank_copy_fit_end_of_bank),
        ct_maketest(bank_copy_masked_address),

        ct_maketest(bank_copymirrored_zero_count),
        ct_maketest(bank_copymirrored_within_single_bank),
        ct_maketest(bank_copymirrored_cross_bank_boundary),
        ct_maketest(bank_copymirrored_cross_middle_bank_boundary),
        ct_maketest(bank_copymirrored_end_of_address_space),
    };

    return ct_makesuite(tests);
}
