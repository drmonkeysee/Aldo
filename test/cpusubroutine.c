//
//  cpusubroutine.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 4/3/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stddef.h>
#include <stdint.h>

//
// MARK: - Subroutine Instructions
//

static void jsr(void *ctx)
{
    uint8_t mem[] = {0x20, 0x5, 0x80, [259] = 0xff, [260] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.s = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(0x8005u, cpu.pc);

    ct_assertequal(0x2u, mem[259]);
    ct_assertequal(0x0u, mem[260]);
    ct_assertequal(2u, cpu.s);
}

static void rts(void *ctx)
{
    uint8_t mem[] = {0x60, 0xff, 0xff, [259] = 0x2, [260] = 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.s = 2;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(4u, cpu.s);
}

//
// MARK: - Test List
//

struct ct_testsuite cpu_subroutine_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(jsr),
        ct_maketest(rts),
    };

    return ct_makesuite(tests);
}
