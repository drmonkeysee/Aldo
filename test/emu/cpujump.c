//
//  cpujump.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/20/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"

#include <stdint.h>

//
// Jump Instructions
//

static void jmp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4c, 0x1, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(0x8001u, cpu.pc);
}

static void jmp_indirect(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6c, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xef, 0xbe};
    cpu.ram = mem;
    cpu.rom = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(0xbeefu, cpu.pc);
}

static void jmp_indirect_pageboundary_bug(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: *supposed* to read from $80FF, $8100
    // but actually ignores the carry and reads from $80FF, $8000.
    uint8_t mem[] = {0x6c, 0xff, 0x80};
    cpu.ram = mem;
    cpu.rom = bigrom;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    // NOTE: pc ends up pointing at address built from $80FF, $8000
    ct_assertequal(0xcafeu, cpu.pc);
}

//
// Test List
//

struct ct_testsuite cpu_jump_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(jmp),
        ct_maketest(jmp_indirect),
        ct_maketest(jmp_indirect_pageboundary_bug),
    };

    return ct_makesuite(tests);
}
