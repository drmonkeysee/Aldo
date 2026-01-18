//
//  cpujump.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/20/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stdint.h>

//
// MARK: - Jump Instructions
//

static void jmp(void *ctx)
{
    uint8_t mem[] = {0x4c, 0x1, 0x80};
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, nullptr);

    auto cycles = exec_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(0x8001u, cpu.pc);
}

static void jmp_indirect(void *ctx)
{
    uint8_t mem[] = {0x6c, 0x1, 0x80},
            abs[] = {0xff, 0xef, 0xbe};
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, abs);

    auto cycles = exec_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(0xbeefu, cpu.pc);
}

static void jmp_indirect_pageboundary_bug(void *ctx)
{
    // NOTE: *supposed* to read from $80FF, $8100
    // but actually ignores the carry and reads from $80FF, $8000.
    uint8_t mem[] = {0x6c, 0xff, 0x80};
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);

    auto cycles = exec_cpu(&cpu);

    ct_assertequal(5, cycles);
    // NOTE: pc ends up pointing at address built from $80FF, $8000
    ct_assertequal(0xcafeu, cpu.pc);
}

//
// MARK: - Test List
//

struct ct_testsuite cpu_jump_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(jmp),
        ct_maketest(jmp_indirect),
        ct_maketest(jmp_indirect_pageboundary_bug),
    };

    return ct_makesuite(tests);
}
