//
//  asm.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "asm.h"
#include "ciny.h"

static void test(void *context)
{
    const char *const err = dis_errstr(10);
    ct_assertfail("Fake Test: %s", err);
}

struct ct_testsuite asm_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(test),
    };

    return ct_makesuite(tests);
}
