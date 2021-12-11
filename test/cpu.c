//
//  cpu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/27/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stddef.h>
#include <stdint.h>

static void powerup_initializes_cpu(void *ctx)
{
    struct mos6502 cpu;

    cpu_powerup(&cpu);

    ct_asserttrue(cpu.signal.irq);
    ct_asserttrue(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_asserttrue(cpu.signal.rw);
    ct_assertfalse(cpu.signal.rdy);
    ct_assertfalse(cpu.signal.sync);
}

static void data_fault(void *ctx)
{
    uint8_t mem[] = {0xad, 0x1f, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_asserttrue(cpu.bflt);
}

static void ram_mirroring(void *ctx)
{
    uint8_t mem[] = {0xad, 0x3, 0x8, 0x45}; // $0803 -> $0003
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

//
// Test List
//

struct ct_testsuite cpu_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_cpu),
        ct_maketest(data_fault),
        ct_maketest(ram_mirroring),
    };

    return ct_makesuite(tests);
}
