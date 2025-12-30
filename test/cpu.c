//
//  cpu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/27/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stdint.h>

static void powerup_initializes_cpu(void *ctx)
{
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, nullptr, nullptr);

    aldo_cpu_powerup(&cpu);

    ct_asserttrue(cpu.signal.irq);
    ct_asserttrue(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.rst);
    ct_asserttrue(cpu.signal.rw);
    ct_assertfalse(cpu.signal.rdy);
    ct_assertfalse(cpu.signal.sync);
    ct_assertfalse(cpu.bflt);
    ct_assertfalse(cpu.presync);
    ct_assertfalse(cpu.detached);
}

static void data_fault(void *ctx)
{
    uint8_t mem[] = {0xad, 0x1f, 0x40};
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, nullptr);

    int cycles = exec_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_asserttrue(cpu.bflt);
}

static void ram_mirroring(void *ctx)
{
    uint8_t mem[] = {0xad, 0x3, 0x8, 0x45}; // $0803 -> $0003
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, nullptr);

    int cycles = exec_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

//
// MARK: - Test List
//

struct ct_testsuite cpu_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_cpu),
        ct_maketest(data_fault),
        ct_maketest(ram_mirroring),
    };

    return ct_makesuite(tests);
}
