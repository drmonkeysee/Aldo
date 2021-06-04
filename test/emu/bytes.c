//
//  bytes.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 6/3/21.
//

#include "ciny.h"
#include "emu/bytes.h"

static void bank_copy_zero(void *ctx)
{
    uint8_t bank[] = {
        0xaa, 0xaa, 0xaa,
        [1021] = 0xff, [1022] = 0xff, [1023] = 0xff,
    };
    uint8_t dest = 0x11;

    const size_t result = bytescopy_bank(bank, 10, 0x0, 0, &dest);

    ct_assertequal(0u, result);
    ct_assertequal(0x11u, dest);
}


//
// Test List
//

struct ct_testsuite bytes_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(bank_copy_zero),
    };

    return ct_makesuite(tests);
}
