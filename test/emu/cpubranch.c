//
//  cpubranch.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/26/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"

#include <stdint.h>

//
// Branch Instructions
//

static void cpu_bcc_nobranch(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x90, 0x5};
    cpu.ram = mem;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);
}

static void cpu_bcc_positive(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x90, 0x5};    // NOTE: $0002 + 5
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(7u, cpu.pc);
}

static void cpu_bcc_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xff, 0xff, 0xff, 0xff, 0x90, 0xfb};   // NOTE: $0006 - 5
    cpu.ram = mem;
    cpu.pc = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(1u, cpu.pc);
}

static void cpu_bcc_positive_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {[250] = 0x90, 0xa};    // NOTE: $00FD + 10
    cpu.ram = mem;
    cpu.pc = 250;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(262u, cpu.pc);
}

static void cpu_bcc_negative_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {[256] = 0x90, 0xf6};   // NOTE: $0102 - 10
    cpu.ram = mem;
    cpu.pc = 256;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(248u, cpu.pc);
}

static void cpu_bcc_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x90, 0x0};    // NOTE: $0002 + 0
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);
}

//
// Test List
//

struct ct_testsuite cpu_branch_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_bcc_nobranch),
        ct_maketest(cpu_bcc_positive),
        ct_maketest(cpu_bcc_negative),
        ct_maketest(cpu_bcc_positive_overflow),
        ct_maketest(cpu_bcc_negative_overflow),
        ct_maketest(cpu_bcc_zero),
    };

    return ct_makesuite(tests);
}
