//
//  chip.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "chip.h"
#include "ciny.h"

static void powerup_initializes_chip(void *ctx)
{
    struct aldo_rp2a03 chip;

    aldo_chip_powerup(&chip);

    ct_assertfalse(chip.put);
}

//
// MARK: - Test List
//

struct ct_testsuite chip_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_chip),
    };

    return ct_makesuite(tests);
}
