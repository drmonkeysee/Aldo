//
//  cpu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/27/21.
//

#include "ciny.h"
#include "emu/cpu.h"
#include "emu/snapshot.h"

#include <stdint.h>

static void setup_cpu(struct mos6502 *cpu)
{
    cpu_powerup(cpu);
    // NOTE: set the cpu to instruction prefetch state
    cpu->pc = 0;
    cpu->signal.rdy = cpu->idone = true;
}

static int clock_cpu(struct mos6502 *cpu)
{
    int cycles = 0;
    do {
        cycles += cpu_cycle(cpu);
    } while (!cpu->idone);
    return cycles;
}

//
// cpu_powerup
//

static void cpu_powerup_initializes_cpu(void *ctx)
{
    struct mos6502 cpu;

    cpu_powerup(&cpu);

    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0u, cpu.a);
    ct_assertequal(0u, cpu.s);
    ct_assertequal(0u, cpu.x);
    ct_assertequal(0u, cpu.y);

    ct_assertequal(0x34u, sn.cpu.status);

    ct_assertequal(-1, cpu.t);

    ct_asserttrue(cpu.signal.irq);
    ct_asserttrue(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_asserttrue(cpu.signal.rw);
    ct_assertfalse(cpu.signal.rdy);
    ct_assertfalse(cpu.signal.sync);

    ct_assertfalse(cpu.idone);
    ct_assertfalse(cpu.dflt);
}

//
// Implied Instructions
//

static void cpu_clc(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x18, 0xff};
    cpu.ram = mem;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    ct_assertfalse(cpu.p.c);
}

static void cpu_cld(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd8, 0xff};
    cpu.ram = mem;
    cpu.p.d = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    ct_assertfalse(cpu.p.d);
}

static void cpu_cli(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x58, 0xff};
    cpu.ram = mem;
    cpu.p.i = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    ct_assertfalse(cpu.p.i);
}

static void cpu_clv(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb8, 0xff};
    cpu.ram = mem;
    cpu.p.v = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    ct_assertfalse(cpu.p.v);
}

static void cpu_nop(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xea, 0xff};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    // NOTE: verify NOP did nothing
    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0u, cpu.a);
    ct_assertequal(0u, cpu.s);
    ct_assertequal(0u, cpu.x);
    ct_assertequal(0u, cpu.y);
    ct_assertequal(0x34u, sn.cpu.status);
}

static void cpu_sec(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x38, 0xff};
    cpu.ram = mem;
    cpu.p.c = false;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    ct_asserttrue(cpu.p.c);
}

static void cpu_sed(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xf8, 0xff};
    cpu.ram = mem;
    cpu.p.d = false;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    ct_asserttrue(cpu.p.d);
}

static void cpu_sei(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x78, 0xff};
    cpu.ram = mem;
    cpu.p.i = false;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(0x1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);
    ct_asserttrue(cpu.idone);

    ct_asserttrue(cpu.p.i);
}

//
// Test List
//

struct ct_testsuite cpu_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_powerup_initializes_cpu),

        ct_maketest(cpu_clc),
        ct_maketest(cpu_cld),
        ct_maketest(cpu_cli),
        ct_maketest(cpu_clv),
        ct_maketest(cpu_nop),
        ct_maketest(cpu_sec),
        ct_maketest(cpu_sed),
        ct_maketest(cpu_sei),
    };

    return ct_makesuite(tests);
}
