//
//  cpu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/27/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"
#include "emu/snapshot.h"

#include <stdint.h>

static void cpu_powerup_initializes_cpu(void *ctx)
{
    struct mos6502 cpu;

    cpu_powerup(&cpu);

    ct_assertequal(0u, cpu.a);
    ct_assertequal(0u, cpu.s);
    ct_assertequal(0u, cpu.x);
    ct_assertequal(0u, cpu.y);

    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0x34u, sn.cpu.status);

    ct_asserttrue(cpu.signal.irq);
    ct_asserttrue(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_asserttrue(cpu.signal.rw);
    ct_assertfalse(cpu.signal.rdy);
    ct_assertfalse(cpu.signal.sync);

    ct_asserttrue(cpu.presync);
    ct_assertfalse(cpu.dflt);
}

static void cpu_data_fault(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x1f, 0x40};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_asserttrue(cpu.dflt);
}

static void cpu_ram_mirroring(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x3, 0x8, 0x45}; // NOTE: $0803 -> $0003
    cpu.ram = mem;

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
        ct_maketest(cpu_powerup_initializes_cpu),
        ct_maketest(cpu_data_fault),
        ct_maketest(cpu_ram_mirroring),
    };

    return ct_makesuite(tests);
}
