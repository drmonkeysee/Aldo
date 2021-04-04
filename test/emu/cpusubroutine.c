//
//  cpusubroutine.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 4/3/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"

#include <stdint.h>

//
// Subroutine Instructions
//

//
// Test List
//

struct ct_testsuite cpu_subroutine_tests(void)
{
    static const struct ct_testcase tests[] = {
    };

    return ct_makesuite(tests);
}
