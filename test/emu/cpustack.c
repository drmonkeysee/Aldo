//
//  cpustack.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 4/1/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"
#include "emu/snapshot.h"

#include <stdint.h>

//
// Stack Instructions
//

static void cpu_pla(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x68, [260] = 0x20};
    cpu.ram = mem;
    cpu.s = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0x20u, cpu.a);
    ct_assertequal(4u, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_pla_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x68, [260] = 0x0};
    cpu.ram = mem;
    cpu.a = 0xa;
    cpu.s = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertequal(4u, cpu.s);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_pla_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x68, [260] = 0xff};
    cpu.ram = mem;
    cpu.s = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertequal(4u, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_pla_wraparound(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x68, [256] = 0x20};
    cpu.ram = mem;
    cpu.s = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0x20u, cpu.a);
    ct_assertequal(0u, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_plp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x28, [260] = 0xaa};
    cpu.ram = mem;
    cpu.s = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0xaau, sn.cpu.status);
    ct_assertequal(4u, cpu.s);
}

static void cpu_plp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x28, [260] = 0x0};
    cpu.ram = mem;
    cpu.s = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0x0u, sn.cpu.status);
    ct_assertequal(4u, cpu.s);
}

static void cpu_plp_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x28, [260] = 0xff};
    cpu.ram = mem;
    cpu.s = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0xffu, sn.cpu.status);
    ct_assertequal(4u, cpu.s);
}

static void cpu_plp_wraparound(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x28, [256] = 0x20};
    cpu.ram = mem;
    cpu.s = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0x20u, sn.cpu.status);
    ct_assertequal(4u, cpu.s);
}

//
// Test List
//

struct ct_testsuite cpu_stack_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_pla),
        ct_maketest(cpu_pla_zero),
        ct_maketest(cpu_pla_negative),
        ct_maketest(cpu_pla_wraparound),

        ct_maketest(cpu_plp),
        ct_maketest(cpu_plp_zero),
        ct_maketest(cpu_plp_ones),
        ct_maketest(cpu_plp_wraparound),
    };

    return ct_makesuite(tests);
}
