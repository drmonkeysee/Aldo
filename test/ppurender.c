//
//  ppurender.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 10/22/24.
//

#include "ciny.h"
#include "ppuhelp.h"

void ppu_render_setup(void **ctx)
{
    void *v;
    ppu_setup(&v);
    *ctx = v;
}

void placeholder(void *ctx)
{
    ct_assertfail("NOT IMPLEMENTED");
}

//
// MARK: - Test List
//

struct ct_testsuite ppu_render_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(placeholder),
    };

    return ct_makesuite_setup_teardown(tests, ppu_render_setup, ppu_teardown);
}
