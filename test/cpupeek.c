//
//  cpupeek.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 12/8/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stddef.h>
#include <stdint.h>

static void end_restores_state(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu, NULL, NULL);
    cpu.pc = 0x8000;
    cpu.a = 0x5;
    cpu.x = 0x1;
    cpu.y = 0xa;
    cpu.signal.irq = true;
    cpu.signal.nmi = false;
    cpu.signal.res = true;
    cpu.signal.rdy = false;
    cpu.irq = NIS_PENDING;
    cpu.nmi = NIS_SERVICED;
    cpu.res = NIS_COMMITTED;

    ct_assertfalse(cpu.detached);

    cpu_ctx *const cctx = cpu_peek_start(&cpu);

    ct_asserttrue(cpu.signal.irq);
    ct_asserttrue(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_asserttrue(cpu.signal.rdy);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
    ct_asserttrue(cpu.detached);

    cpu.pc = 0xc000;
    cpu.a = 0x40;
    cpu.x = 0x4;
    cpu.y = 0xc;

    cpu_peek_end(&cpu, cctx);

    ct_asserttrue(cpu.signal.irq);
    ct_assertfalse(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_assertfalse(cpu.signal.rdy);
    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertfalse(cpu.detached);

    ct_assertequal(0x8000u, cpu.pc);
    ct_assertequal(0x5u, cpu.a);
    ct_assertequal(0x1u, cpu.x);
    ct_assertequal(0xau, cpu.y);
}

static void end_retains_detached_state(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu, NULL, NULL);
    cpu.detached = true;

    ct_asserttrue(cpu.detached);

    cpu_ctx *const cctx = cpu_peek_start(&cpu);

    ct_asserttrue(cpu.detached);

    cpu_peek_end(&cpu, cctx);

    ct_asserttrue(cpu.detached);
}

static void irq_ignored(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);

    // NOTE: throw away return value, no need to clean up peek state in test
    (void)cpu_peek_start(&cpu);
    cpu.s = 0xff;
    cpu.p.i = false;
    cpu.signal.irq = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_PENDING, (int)cpu.irq);
}

static void nmi_ignored(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);

    // NOTE: throw away return value, no need to clean up peek state in test
    (void)cpu_peek_start(&cpu);
    cpu.s = 0xff;
    cpu.signal.nmi = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
}

static void res_not_ignored(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);

    // NOTE: throw away return value, no need to clean up peek state in test
    (void)cpu_peek_start(&cpu);
    cpu.s = 0xff;
    cpu.signal.res = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
}

static void writes_ignored(void *ctx)
{
    // NOTE: STA $04 (0x20)
    uint8_t mem[] = {0x85, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    // NOTE: throw away return value, no need to clean up peek state in test
    (void)cpu_peek_start(&cpu);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);
    ct_assertequal(0x10u, cpu.a);
    ct_assertequal(0x20u, mem[4]);
    ct_assertequal(0x20u, cpu.databus);
}

//
// Test List
//

struct ct_testsuite cpu_peek_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(end_restores_state),
        ct_maketest(end_retains_detached_state),
        ct_maketest(irq_ignored),
        ct_maketest(nmi_ignored),
        ct_maketest(res_not_ignored),
        ct_maketest(writes_ignored),
    };

    return ct_makesuite(tests);
}
