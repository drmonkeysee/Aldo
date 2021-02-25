//
//  asm.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "ciny.h"

static void test(void *context)
{
    ct_assertfail("Fake Test!");
}

struct ct_testsuite asm_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(test),
    };

    return ct_makesuite(tests);
}
