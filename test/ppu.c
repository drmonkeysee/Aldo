//
//  ppu.c
//  Tests
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ciny.h"
#include "ppu.h"

static void powerup_initializes_ppu(void *ctx)
{
    struct rp2c02 ppu;

    ppu_powerup(&ppu);

    ct_assertfalse(ppu.odd);
    ct_assertfalse(ppu.w);
}

//
// Test List
//

struct ct_testsuite ppu_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_ppu),
    };

    return ct_makesuite(tests);
}
