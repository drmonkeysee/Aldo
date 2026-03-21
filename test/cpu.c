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
    ct_asserttrue(cpu.signal.rdy);
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

    auto cycles = exec_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_asserttrue(cpu.bflt);
}

static void ram_mirroring(void *ctx)
{
    uint8_t mem[] = {0xad, 0x3, 0x8, 0x45}; // $0803 -> $0003
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, nullptr);

    auto cycles = exec_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ready_low_on_read(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};    // ADC #6
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, nullptr);

    auto cycle = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycle);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);

    cpu.signal.rdy = false;

    cycle = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycle);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);

    cycle = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycle);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);

    cpu.signal.rdy = true;

    cycle = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycle);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x6u, cpu.databus);
    ct_assertequal(0x2u, cpu.pc);
    ct_assertequal(0x1u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);
}

static void ready_low_on_write(void *ctx)
{
    uint8_t mem[] = {
        0x8d, 0x4, 0x2, // STA $0204
        0x69, 0x6,      // ADC #6
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, nullptr);
    cpu.a = 0xc;

    auto cycles = exec_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(0x8du, cpu.opc);
    ct_assertequal(0xcu, cpu.databus);
    ct_assertequal(0x3u, cpu.pc);
    ct_assertequal(0x204u, cpu.addrbus);
    ct_assertequal(0xcu, mem[516]);
    ct_assertfalse(cpu.signal.rw);

    cpu.signal.rdy = false;

    // cpu advances to next read cycle despite RDY being low
    cycles = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycles);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0x4u, cpu.pc);
    ct_assertequal(0x3u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);

    cycles = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycles);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0x4u, cpu.pc);
    ct_assertequal(0x3u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);

    cpu.signal.rdy = true;

    cycles = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycles);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x6u, cpu.databus);
    ct_assertequal(0x5u, cpu.pc);
    ct_assertequal(0x4u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);
}

static void ready_low_on_init(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};    // ADC #6
    struct aldo_mos6502 cpu;
    setup_cpu(&cpu, mem, nullptr);
    cpu.addrbus = cpu.databus = 0x0;
    cpu.signal.rdy = false;

    auto cycle = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycle);
    ct_assertequal(0u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0u, cpu.pc);
    ct_assertequal(0u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);
    ct_asserttrue(cpu.presync);

    cycle = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycle);
    ct_assertequal(0u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0u, cpu.pc);
    ct_assertequal(0u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);
    ct_asserttrue(cpu.presync);

    cpu.signal.rdy = true;

    cycle = aldo_cpu_cycle(&cpu);

    ct_assertequal(1, cycle);
    ct_assertequal(0x69u, cpu.opc);
    ct_assertequal(0x69u, cpu.databus);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0u, cpu.addrbus);
    ct_asserttrue(cpu.signal.rw);
    ct_assertfalse(cpu.presync);
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
        ct_maketest(ready_low_on_read),
        ct_maketest(ready_low_on_write),
        ct_maketest(ready_low_on_init),
    };

    return ct_makesuite(tests);
}
