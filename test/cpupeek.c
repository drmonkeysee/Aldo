//
//  cpupeek.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 12/8/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"
#include "ctrlsignal.h"

#include <stddef.h>
#include <stdint.h>

static void end_restores_state(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu, NULL, NULL);
    cpu.pc = 0x8000;
    cpu.a = 5;
    cpu.x = 1;
    cpu.y = 0xa;
    cpu.signal.irq = true;
    cpu.signal.nmi = false;
    cpu.signal.res = true;
    cpu.signal.rdy = false;
    cpu.irq = CSGS_PENDING;
    cpu.nmi = CSGS_SERVICED;
    cpu.res = CSGS_COMMITTED;

    ct_assertfalse(cpu.detached);

    struct mos6502 bak;
    cpu_peek_start(&cpu, &bak);

    ct_asserttrue(cpu.signal.irq);
    ct_asserttrue(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_asserttrue(cpu.signal.rdy);
    ct_assertequal(CSGS_CLEAR, (int)cpu.irq);
    ct_assertequal(CSGS_CLEAR, (int)cpu.nmi);
    ct_assertequal(CSGS_CLEAR, (int)cpu.res);
    ct_asserttrue(cpu.detached);

    cpu.pc = 0xc000;
    cpu.a = 0x40;
    cpu.x = 4;
    cpu.y = 0xc;

    cpu_peek_end(&cpu, &bak);

    ct_asserttrue(cpu.signal.irq);
    ct_assertfalse(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_assertfalse(cpu.signal.rdy);
    ct_assertequal(CSGS_PENDING, (int)cpu.irq);
    ct_assertequal(CSGS_SERVICED, (int)cpu.nmi);
    ct_assertequal(CSGS_COMMITTED, (int)cpu.res);
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

    struct mos6502 bak;
    cpu_peek_start(&cpu, &bak);

    ct_asserttrue(cpu.detached);

    cpu_peek_end(&cpu, &bak);

    ct_asserttrue(cpu.detached);
}

static void irq_ignored(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);

    cpu_peek_start(&cpu, NULL);
    cpu.s = 0xff;
    cpu.p.i = false;
    cpu.signal.irq = false;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(CSGS_PENDING, (int)cpu.irq);
}

static void nmi_ignored(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);

    cpu_peek_start(&cpu, NULL);
    cpu.s = 0xff;
    cpu.signal.nmi = false;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(CSGS_PENDING, (int)cpu.nmi);
}

static void res_not_ignored(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);

    cpu_peek_start(&cpu, NULL);
    cpu.s = 0xff;
    cpu.signal.res = false;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);
    ct_assertequal(CSGS_COMMITTED, (int)cpu.res);
}

static void writes_ignored(void *ctx)
{
    // NOTE: STA $04 (0x20)
    uint8_t mem[] = {0x85, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu_peek_start(&cpu, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);
    ct_assertequal(0x10u, cpu.a);
    ct_assertequal(0x20u, mem[4]);
    ct_assertequal(0x20u, cpu.databus);
}

static void peek_immediate(void *ctx)
{
    // NOTE: LDA #$10
    uint8_t mem[] = {0xa9, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_IMM, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x1u, result.finaladdr);
    ct_assertequal(0x10u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_zeropage(void *ctx)
{
    // NOTE: LDA $04
    uint8_t mem[] = {0xa5, 0x4, 0x0, 0x0, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_ZP, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x4u, result.finaladdr);
    ct_assertequal(0x20u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_zp_indexed(void *ctx)
{
    // NOTE: LDA $03,X
    uint8_t mem[] = {0xb5, 0x3, 0x0, 0x0, 0x0, 0x30};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu.x = 2;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_ZPX, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x5u, result.finaladdr);
    ct_assertequal(0x30u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_indexed_indirect(void *ctx)
{
    // NOTE: LDA ($02,X)
    uint8_t mem[] = {0xa1, 0x2, 0x0, 0x0, 0x2, 0x1, [258] = 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu.x = 2;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_INDX, (int)result.mode);
    ct_assertequal(0x4u, result.interaddr);
    ct_assertequal(0x102u, result.finaladdr);
    ct_assertequal(0x40u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_indirect_indexed(void *ctx)
{
    // NOTE: LDA ($02),Y
    uint8_t mem[] = {0xb1, 0x2, 0x2, 0x1, [263] = 0x60};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu.y = 5;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_INDY, (int)result.mode);
    ct_assertequal(0x102u, result.interaddr);
    ct_assertequal(0x107u, result.finaladdr);
    ct_assertequal(0x60u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_absolute(void *ctx)
{
    // NOTE: LDA $0102
    uint8_t mem[] = {0xad, 0x2, 0x1, [258] = 0x70};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_ABS, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x102u, result.finaladdr);
    ct_assertequal(0x70u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_absolute_indexed(void *ctx)
{
    // NOTE: LDA $0102,X
    uint8_t mem[] = {0xbd, 0x2, 0x1, [268] = 0x70};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu.x = 0xa;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_ABSX, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x10cu, result.finaladdr);
    ct_assertequal(0x70u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_branch(void *ctx)
{
    // NOTE: BEQ +5
    uint8_t mem[] = {0xf0, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x55};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.p.z = true;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_BCH, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x7u, result.finaladdr);
    ct_assertequal(0x0u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_branch_forced(void *ctx)
{
    // NOTE: BEQ +5
    uint8_t mem[] = {0xf0, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x55};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.p.z = false;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_BCH, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x7u, result.finaladdr);
    ct_assertequal(0x0u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_absolute_indirect(void *ctx)
{
    // NOTE: LDA ($0102)
    uint8_t mem[] = {0x6c, 0x2, 0x1, [258] = 0x5, 0x2, [517] = 80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu.x = 0xa;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_JIND, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x205u, result.finaladdr);
    ct_assertequal(0x2u, result.data);
    ct_assertfalse(result.busfault);
}

static void peek_jam(void *ctx)
{
    // NOTE: JAM
    uint8_t mem[] = {0x02, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_JAM, (int)result.mode);
}

static void peek_busfault(void *ctx)
{
    // NOTE: LDA $4002
    uint8_t mem[] = {0xad, 0x2, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.a = 0x10;
    cpu_peek_start(&cpu, NULL);

    const struct peekresult result = cpu_peek(&cpu, 0x0);

    ct_assertequal(AM_ABS, (int)result.mode);
    ct_assertequal(0x0u, result.interaddr);
    ct_assertequal(0x4002u, result.finaladdr);
    ct_asserttrue(result.busfault);
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
        ct_maketest(peek_immediate),
        ct_maketest(peek_zeropage),
        ct_maketest(peek_zp_indexed),
        ct_maketest(peek_indexed_indirect),
        ct_maketest(peek_indirect_indexed),
        ct_maketest(peek_absolute),
        ct_maketest(peek_absolute_indexed),
        ct_maketest(peek_branch),
        ct_maketest(peek_branch_forced),
        ct_maketest(peek_absolute_indirect),
        ct_maketest(peek_jam),
        ct_maketest(peek_busfault),
    };

    return ct_makesuite(tests);
}
