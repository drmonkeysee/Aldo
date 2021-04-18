//
//  cpuinterrupt.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 4/14/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"
#include "emu/snapshot.h"

#include <stdint.h>

//
// Interrupt Signals
//

static void clear_on_startup(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void irq_latched(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
}

static void irq_too_short(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu.signal.irq = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
}

static void irq_level_dependent(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);

    cpu.signal.irq = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
}

static void irq_detect_duplicate(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    // NOTE: simulate irq held active during and after servicing interrupt
    cpu.signal.irq = false;
    cpu.irq = NIS_COMMITTED;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    cpu.irq = NIS_CLEAR;
    cpu_cycle(&cpu);

    // NOTE: if irq is still held active after servicing it'll
    // be detected again.
    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
}

static void nmi_latched(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
}

static void nmi_too_short(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu.signal.nmi = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
}

static void nmi_edge_persist(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);

    cpu.signal.nmi = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
}

static void nmi_serviced_only_clears_on_inactive(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    // NOTE: simulate nmi held active during and after servicing interrupt
    cpu.signal.nmi = false;
    cpu.nmi = NIS_COMMITTED;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cpu.nmi = NIS_SERVICED;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);

    // NOTE: nmi cannot be detected again until line goes inactive
    cpu.signal.nmi = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
}

static void res_latched(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);
}

static void res_too_short(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void res_commits_and_holds_cpu(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x10, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(2u, cpu.pc);
}

//
// Test List
//

struct ct_testsuite cpu_interrupt_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(clear_on_startup),
        ct_maketest(irq_latched),
        ct_maketest(irq_too_short),
        ct_maketest(irq_level_dependent),
        ct_maketest(irq_detect_duplicate),
        ct_maketest(nmi_latched),
        ct_maketest(nmi_too_short),
        ct_maketest(nmi_edge_persist),
        ct_maketest(nmi_serviced_only_clears_on_inactive),
        ct_maketest(res_latched),
        ct_maketest(res_too_short),
        ct_maketest(res_commits_and_holds_cpu),
    };

    return ct_makesuite(tests);
}
