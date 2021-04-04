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

static void cpu_jsr(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x20, 0x5, 0x80, [259] = 0xff, [260] = 0xff};
    cpu.ram = mem;
    cpu.s = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(0x8005u, cpu.pc);

    ct_assertequal(0x2u, mem[259]);
    ct_assertequal(0x0u, mem[260]);
    ct_assertequal(2u, cpu.s);
}

static void cpu_rts(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x60, 0xff, 0xff, [259] = 0x2, [260] = 0x0};
    cpu.ram = mem;
    cpu.s = 2;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(0x3u, cpu.pc);

    ct_assertequal(4u, cpu.s);
}

//
// Test List
//

struct ct_testsuite cpu_subroutine_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_jsr),
        ct_maketest(cpu_rts),
    };

    return ct_makesuite(tests);
}
