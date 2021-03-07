//
//  cpuimmediate.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"

#include <stdint.h>

//
// Immediate Instructions
//

static void cpu_and_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x29, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x29, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x29, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x49, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x49, 0xaa};
    cpu.a = 0xaa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x49, 0xc};
    cpu.a = 0xfa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_lda_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldx_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa2, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa2, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa2, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldy_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa0, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa0, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa0, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Test List
//

struct ct_testsuite cpu_immediate_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_and_imm),
        ct_maketest(cpu_and_imm_zero),
        ct_maketest(cpu_and_imm_negative),
        ct_maketest(cpu_eor_imm),
        ct_maketest(cpu_eor_imm_zero),
        ct_maketest(cpu_eor_imm_negative),
        ct_maketest(cpu_lda_imm),
        ct_maketest(cpu_lda_imm_zero),
        ct_maketest(cpu_lda_imm_negative),
        ct_maketest(cpu_ldx_imm),
        ct_maketest(cpu_ldx_imm_zero),
        ct_maketest(cpu_ldx_imm_negative),
        ct_maketest(cpu_ldy_imm),
        ct_maketest(cpu_ldy_imm_zero),
        ct_maketest(cpu_ldy_imm_negative),
        ct_maketest(cpu_ora_imm),
        ct_maketest(cpu_ora_imm_zero),
        ct_maketest(cpu_ora_imm_negative),
    };

    return ct_makesuite(tests);
}
