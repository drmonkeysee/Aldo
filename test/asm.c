//
//  asm.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "asm.h"
#include "ciny.h"

static void dis_errstr_returns_known_err(void *context)
{
    const char *const err = dis_errstr(ASM_FMT_FAIL);
    ct_assertequalstr("OUTPUT FAIL", err);
}

static void dis_errstr_returns_unknown_err(void *context)
{
    const char *const err = dis_errstr(10);
    ct_assertequalstr("UNKNOWN ERR", err);
}

struct ct_testsuite asm_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(dis_errstr_returns_known_err),
        ct_maketest(dis_errstr_returns_unknown_err),
    };

    return ct_makesuite(tests);
}
