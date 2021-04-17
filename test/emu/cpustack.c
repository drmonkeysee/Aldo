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

static void pha(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x48, [260] = 0x0};
    cpu.ram = mem;
    cpu.a = 0x20;
    cpu.s = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0x20u, mem[260]);
    ct_assertequal(3u, cpu.s);
}

static void pha_wraparound(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x48, [256] = 0x0};
    cpu.ram = mem;
    cpu.a = 0x20;
    cpu.s = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0x20u, mem[256]);
    ct_assertequal(0xffu, cpu.s);
}

static void php(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x8, [260] = 0x0};
    cpu.ram = mem;
    cpu.p.n = true;
    cpu.p.z = true;
    cpu.s = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0xb6u, mem[260]);
    ct_assertequal(3u, cpu.s);
}

static void php_wraparound(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x8, [256] = 0x0};
    cpu.ram = mem;
    cpu.p.n = true;
    cpu.p.z = true;
    cpu.s = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(1u, cpu.pc);

    ct_assertequal(0xb6u, mem[256]);
    ct_assertequal(0xffu, cpu.s);
}

static void pla(void *ctx)
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

static void pla_zero(void *ctx)
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

static void pla_negative(void *ctx)
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

static void pla_wraparound(void *ctx)
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

static void plp(void *ctx)
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
    ct_assertequal(0xbau, sn.cpu.status);
    ct_assertequal(4u, cpu.s);
}

static void plp_zero(void *ctx)
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
    ct_assertequal(0x30u, sn.cpu.status);
    ct_assertequal(4u, cpu.s);
}

static void plp_ones(void *ctx)
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

static void plp_wraparound(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x28, [256] = 0x25};
    cpu.ram = mem;
    cpu.s = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(1u, cpu.pc);

    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0x35u, sn.cpu.status);
    ct_assertequal(0u, cpu.s);
}

//
// Test List
//

struct ct_testsuite cpu_stack_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(pha),
        ct_maketest(pha_wraparound),

        ct_maketest(php),
        ct_maketest(php_wraparound),

        ct_maketest(pla),
        ct_maketest(pla_zero),
        ct_maketest(pla_negative),
        ct_maketest(pla_wraparound),

        ct_maketest(plp),
        ct_maketest(plp_zero),
        ct_maketest(plp_ones),
        ct_maketest(plp_wraparound),
    };

    return ct_makesuite(tests);
}
