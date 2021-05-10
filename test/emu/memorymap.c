//
//  memorymap.c
//  AldoTests
//
//  Created by Brandon Stansbury on 5/9/21.
//

#include "ciny.h"
#include "emu/memorymap.h"

void temp_test(void *ctx)
{
    memmap *m = memmap_new();

    ct_assertnull(m);
}

//
// Test List
//

struct ct_testsuite memorymap_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(temp_test),
    };

    return ct_makesuite(tests);
}
