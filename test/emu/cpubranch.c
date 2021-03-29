//
//  cpubranch.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/26/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"
#include "emu/traits.h"

#include <stdint.h>
#include <stdlib.h>

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

static void cpu_bcc_positive_wraparound(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: 32k rom, starting at $8000 to set up $FFFC + 10
    uint8_t *const rom = calloc(0x8000, sizeof *rom);
    rom[0xfffa & CpuCartAddrMask] = 0x90;
    rom[0xfffb & CpuCartAddrMask] = 0xa;
    cpu.cart = rom;
    cpu.pc = 0xfffa;

    const int cycles = clock_cpu(&cpu);

    free(rom);
    ct_assertequal(4, cycles);
    ct_assertequal(6u, cpu.pc);
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

static void cpu_bcc_negative_wraparound(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x90, 0xf6};   // NOTE: $0002 - 10
    cpu.ram = mem;
    // NOTE: 32k rom, starting at $8000 to allow reads of wraparound addresses
    uint8_t *const rom = calloc(0x8000, sizeof *rom);
    cpu.cart = rom;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(0xfff8u, cpu.pc);
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
        ct_maketest(cpu_bcc_positive_wraparound),
        ct_maketest(cpu_bcc_negative_overflow),
        ct_maketest(cpu_bcc_negative_wraparound),
        ct_maketest(cpu_bcc_zero),
    };

    return ct_makesuite(tests);
}
