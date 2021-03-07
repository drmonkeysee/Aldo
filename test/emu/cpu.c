//
//  cpu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/27/21.
//

#include "ciny.h"
#include "emu/cpu.h"
#include "emu/snapshot.h"
#include "emu/traits.h"

#include <stdint.h>

// One page of rom + extra to test page boundary addressing
static const uint8_t bigrom[] = {
    // Start of page; 256 0xffs
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    // End of page
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
};

static void setup_cpu(struct mos6502 *cpu)
{
    cpu_powerup(cpu);
    // Set the cpu ready to read instruction at 0x0
    cpu->pc = 0;
    cpu->signal.rdy = true;
}

static int clock_cpu(struct mos6502 *cpu)
{
    int cycles = 0;
    do {
        cycles += cpu_cycle(cpu);
        // Catch instructions that run longer than possible
        ct_asserttrue(cycles <= MaxCycleCount);
    } while (!cpu->presync);
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

    ct_asserttrue(cpu.signal.irq);
    ct_asserttrue(cpu.signal.nmi);
    ct_asserttrue(cpu.signal.res);
    ct_asserttrue(cpu.signal.rw);
    ct_assertfalse(cpu.signal.rdy);
    ct_assertfalse(cpu.signal.sync);

    ct_asserttrue(cpu.presync);
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
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

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
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

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
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

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
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertfalse(cpu.p.v);
}

static void cpu_dex(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xca, 0xff};
    cpu.ram = mem;
    cpu.x = 5;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(4u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dex_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xca, 0xff};
    cpu.ram = mem;
    cpu.x = 1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dex_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xca, 0xff};
    cpu.ram = mem;
    cpu.x = 0;
    cpu.p.z = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_dey(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x88, 0xff};
    cpu.ram = mem;
    cpu.y = 5;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(4u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dey_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x88, 0xff};
    cpu.ram = mem;
    cpu.y = 1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dey_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x88, 0xff};
    cpu.ram = mem;
    cpu.y = 0;
    cpu.p.z = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_inx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe8, 0xff};
    cpu.ram = mem;
    cpu.x = 5;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(6u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_inx_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe8, 0xff};
    cpu.ram = mem;
    cpu.x = 0xff;
    cpu.p.n = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_inx_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe8, 0xff};
    cpu.ram = mem;
    cpu.x = 0x7f;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_iny(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc8, 0xff};
    cpu.ram = mem;
    cpu.y = 5;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(6u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_iny_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc8, 0xff};
    cpu.ram = mem;
    cpu.y = 0xff;
    cpu.p.n = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_iny_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc8, 0xff};
    cpu.ram = mem;
    cpu.y = 0x7f;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_nop(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xea, 0xff};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    // Verify NOP did nothing
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
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

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
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

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
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_asserttrue(cpu.p.i);
}

static void cpu_tax(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xaa, 0xff};
    cpu.ram = mem;
    cpu.a = 7;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(7u, cpu.x);
    ct_assertequal(cpu.a, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tax_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xaa, 0xff};
    cpu.ram = mem;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.x);
    ct_assertequal(cpu.a, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tax_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xaa, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.x);
    ct_assertequal(cpu.a, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_tay(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa8, 0xff};
    cpu.ram = mem;
    cpu.a = 7;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(7u, cpu.y);
    ct_assertequal(cpu.a, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tay_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa8, 0xff};
    cpu.ram = mem;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.y);
    ct_assertequal(cpu.a, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tay_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa8, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.y);
    ct_assertequal(cpu.a, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_tsx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xba, 0xff};
    cpu.ram = mem;
    cpu.s = 7;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(7u, cpu.x);
    ct_assertequal(cpu.s, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tsx_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xba, 0xff};
    cpu.ram = mem;
    cpu.s = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.x);
    ct_assertequal(cpu.s, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tsx_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xba, 0xff};
    cpu.ram = mem;
    cpu.s = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.x);
    ct_assertequal(cpu.s, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_txa(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x8a, 0xff};
    cpu.ram = mem;
    cpu.x = 7;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(7u, cpu.a);
    ct_assertequal(cpu.x, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_txa_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x8a, 0xff};
    cpu.ram = mem;
    cpu.x = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.a);
    ct_assertequal(cpu.x, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_txa_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x8a, 0xff};
    cpu.ram = mem;
    cpu.x = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.a);
    ct_assertequal(cpu.x, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_txs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9a, 0xff};
    cpu.ram = mem;
    cpu.x = 7;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(7u, cpu.s);
    ct_assertequal(cpu.x, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_txs_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9a, 0xff};
    cpu.ram = mem;
    cpu.x = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.s);
    ct_assertequal(cpu.x, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_txs_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9a, 0xff};
    cpu.ram = mem;
    cpu.x = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.s);
    ct_assertequal(cpu.x, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tya(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x98, 0xff};
    cpu.ram = mem;
    cpu.y = 7;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(7u, cpu.a);
    ct_assertequal(cpu.y, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tya_to_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x98, 0xff};
    cpu.ram = mem;
    cpu.y = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0u, cpu.a);
    ct_assertequal(cpu.y, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_tya_to_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x98, 0xff};
    cpu.ram = mem;
    cpu.y = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.a);
    ct_assertequal(cpu.y, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Immediate Instructions
//

static void cpu_and_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x29, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x29, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_imm_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x29, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x49, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x49, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_imm_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x49, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa9, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldx_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa2, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa2, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa2, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldy_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa0, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa0, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_imm_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa0, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_imm(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_imm_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_imm_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x9, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Zero-Page Instructions
//

static void cpu_and_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x25, 0x4, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x25, 0x4, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_zp_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x25, 0x4, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x45, 0x4, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x45, 0x4, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_zp_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x45, 0x4, 0xff, 0xff, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa5, 0x4, 0xff, 0xff, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa5, 0x4, 0xff, 0xff, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_zp_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa5, 0x4, 0xff, 0xff, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldx_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa6, 0x4, 0xff, 0xff, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa6, 0x4, 0xff, 0xff, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_zp_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa6, 0x4, 0xff, 0xff, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldy_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa4, 0x4, 0xff, 0xff, 0x45};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa4, 0x4, 0xff, 0xff, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_zp_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa4, 0x4, 0xff, 0xff, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5, 0x4, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5, 0x4, 0xff, 0xff, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_zp_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5, 0x4, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Zero-Page,X Instructions
//

static void cpu_and_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x35, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_zpx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x35, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_zpx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x35, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_zpx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x35, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x55, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_zpx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x55, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_zpx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x55, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_zpx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x55, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xd8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_lda_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x56u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_zpx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_zpx_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_lda_zpx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb5, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb4, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x56u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_zpx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb4, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_zpx_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb4, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldy_zpx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb4, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x15, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_zpx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x15, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_zpx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x15, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_zpx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x15, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Zero-Page,Y Instructions
//

static void cpu_ldx_zpy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.y = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x56u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_zpy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.y = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_zpy_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.y = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldx_zpy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb6, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.y = 0xff;   // Wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

//
// (Indirect,X) Instructions
//

static void cpu_and_indx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x21, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_indx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x21, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_indx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x21, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_indx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x21, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_indx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x41, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_indx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x41, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_indx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x41, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_indx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x41, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_indx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_indx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_indx_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_lda_indx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa1, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80, 0x22};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_indx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_indx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_indx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_indx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// (Indirect),Y Instructions
//

static void cpu_and_indy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x31, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_indy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x31, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_indy_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x31, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_indy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x31, 0x2, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_indy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x51, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_indy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x51, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_indy_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x51, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_indy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x51, 0x2, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_indy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb1, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_indy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb1, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_indy_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb1, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_lda_indy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb1, 0x2, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_indy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x11, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_indy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x11, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_indy_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x11, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_indy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x11, 0x2, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Absolute Instructions
//

static void cpu_and_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_abs_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_abs_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2d,  0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_abs_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_abs_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4d,  0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_abs_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_abs_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldx_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xae, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_abs_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xae, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_abs_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xae, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldy_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xac, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_abs_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xac, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_abs_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xac, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_abs_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_abs_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd,  0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Absolute,X Instructions
//

static void cpu_and_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x3d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_absx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x3d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_absx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x3d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_absx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x3d, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xa2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_absx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_absx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_absx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5d, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_absx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_absx_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_lda_absx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbd, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldy_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbc, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_absx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbc, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldy_absx_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbc, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldy_absx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbc, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_absx_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_absx_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_absx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x1d, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Absolute,Y Instructions
//

static void cpu_and_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x39, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_and_absy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x39, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_absy_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x39, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_absy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x39, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xa2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x59, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_absy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x59, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xaa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xaau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_absy_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x59, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0xff;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_eor_absy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x59, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb9, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_absy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb9, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lda_absy_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb9, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_lda_absy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb9, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldx_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbe, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_absy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbe, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ldx_absy_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbe, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ldx_absy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xbe, 0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x19, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_absy_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x19, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0x0;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_ora_absy_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x19, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xff};
    cpu.a = 0x45;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_ora_absy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x19, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Additional Tests
//

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
    uint8_t mem[] = {0xad, 0x3, 0x8, 0x45}; // $0803 -> $0003
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

        ct_maketest(cpu_clc),
        ct_maketest(cpu_cld),
        ct_maketest(cpu_cli),
        ct_maketest(cpu_clv),
        ct_maketest(cpu_dex),
        ct_maketest(cpu_dex_to_zero),
        ct_maketest(cpu_dex_to_negative),
        ct_maketest(cpu_dey),
        ct_maketest(cpu_dey_to_zero),
        ct_maketest(cpu_dey_to_negative),
        ct_maketest(cpu_inx),
        ct_maketest(cpu_inx_to_zero),
        ct_maketest(cpu_inx_to_negative),
        ct_maketest(cpu_iny),
        ct_maketest(cpu_iny_to_zero),
        ct_maketest(cpu_iny_to_negative),
        ct_maketest(cpu_nop),
        ct_maketest(cpu_sec),
        ct_maketest(cpu_sed),
        ct_maketest(cpu_sei),
        ct_maketest(cpu_tax),
        ct_maketest(cpu_tax_to_zero),
        ct_maketest(cpu_tax_to_negative),
        ct_maketest(cpu_tay),
        ct_maketest(cpu_tay_to_zero),
        ct_maketest(cpu_tay_to_negative),
        ct_maketest(cpu_tsx),
        ct_maketest(cpu_tsx_to_zero),
        ct_maketest(cpu_tsx_to_negative),
        ct_maketest(cpu_txa),
        ct_maketest(cpu_txa_to_zero),
        ct_maketest(cpu_txa_to_negative),
        ct_maketest(cpu_txs),
        ct_maketest(cpu_txs_to_zero),
        ct_maketest(cpu_txs_to_negative),
        ct_maketest(cpu_tya),
        ct_maketest(cpu_tya_to_zero),
        ct_maketest(cpu_tya_to_negative),

        ct_maketest(cpu_and_imm),
        ct_maketest(cpu_and_imm_zero),
        ct_maketest(cpu_and_imm_ones),
        ct_maketest(cpu_eor_imm),
        ct_maketest(cpu_eor_imm_zero),
        ct_maketest(cpu_eor_imm_ones),
        ct_maketest(cpu_lda_imm),
        ct_maketest(cpu_lda_imm_zero),
        ct_maketest(cpu_lda_imm_negative),
        ct_maketest(cpu_ldx_imm),
        ct_maketest(cpu_ldx_imm_zero),
        ct_maketest(cpu_ldx_imm_negative),
        ct_maketest(cpu_ldy_imm),
        ct_maketest(cpu_ldy_imm_zero),
        ct_maketest(cpu_ldy_imm_negative),
        ct_maketest(cpu_ora_imm),
        ct_maketest(cpu_ora_imm_zero),
        ct_maketest(cpu_ora_imm_ones),

        ct_maketest(cpu_and_zp),
        ct_maketest(cpu_and_zp_zero),
        ct_maketest(cpu_and_zp_ones),
        ct_maketest(cpu_eor_zp),
        ct_maketest(cpu_eor_zp_zero),
        ct_maketest(cpu_eor_zp_ones),
        ct_maketest(cpu_lda_zp),
        ct_maketest(cpu_lda_zp_zero),
        ct_maketest(cpu_lda_zp_negative),
        ct_maketest(cpu_ldx_zp),
        ct_maketest(cpu_ldx_zp_zero),
        ct_maketest(cpu_ldx_zp_negative),
        ct_maketest(cpu_ldy_zp),
        ct_maketest(cpu_ldy_zp_zero),
        ct_maketest(cpu_ldy_zp_negative),
        ct_maketest(cpu_ora_zp),
        ct_maketest(cpu_ora_zp_zero),
        ct_maketest(cpu_ora_zp_ones),

        ct_maketest(cpu_and_zpx),
        ct_maketest(cpu_and_zpx_zero),
        ct_maketest(cpu_and_zpx_ones),
        ct_maketest(cpu_and_zpx_overflow),
        ct_maketest(cpu_eor_zpx),
        ct_maketest(cpu_eor_zpx_zero),
        ct_maketest(cpu_eor_zpx_ones),
        ct_maketest(cpu_eor_zpx_overflow),
        ct_maketest(cpu_lda_zpx),
        ct_maketest(cpu_lda_zpx_zero),
        ct_maketest(cpu_lda_zpx_negative),
        ct_maketest(cpu_lda_zpx_overflow),
        ct_maketest(cpu_ldy_zpx),
        ct_maketest(cpu_ldy_zpx_zero),
        ct_maketest(cpu_ldy_zpx_negative),
        ct_maketest(cpu_ldy_zpx_overflow),
        ct_maketest(cpu_ora_zpx),
        ct_maketest(cpu_ora_zpx_zero),
        ct_maketest(cpu_ora_zpx_ones),
        ct_maketest(cpu_ora_zpx_overflow),

        ct_maketest(cpu_ldx_zpy),
        ct_maketest(cpu_ldx_zpy_zero),
        ct_maketest(cpu_ldx_zpy_negative),
        ct_maketest(cpu_ldx_zpy_overflow),

        ct_maketest(cpu_and_indx),
        ct_maketest(cpu_and_indx_zero),
        ct_maketest(cpu_and_indx_ones),
        ct_maketest(cpu_and_indx_overflow),
        ct_maketest(cpu_eor_indx),
        ct_maketest(cpu_eor_indx_zero),
        ct_maketest(cpu_eor_indx_ones),
        ct_maketest(cpu_eor_indx_overflow),
        ct_maketest(cpu_lda_indx),
        ct_maketest(cpu_lda_indx_zero),
        ct_maketest(cpu_lda_indx_negative),
        ct_maketest(cpu_lda_indx_overflow),
        ct_maketest(cpu_ora_indx),
        ct_maketest(cpu_ora_indx_zero),
        ct_maketest(cpu_ora_indx_ones),
        ct_maketest(cpu_ora_indx_overflow),

        ct_maketest(cpu_and_indy),
        ct_maketest(cpu_and_indy_zero),
        ct_maketest(cpu_and_indy_ones),
        ct_maketest(cpu_and_indy_overflow),
        ct_maketest(cpu_eor_indy),
        ct_maketest(cpu_eor_indy_zero),
        ct_maketest(cpu_eor_indy_ones),
        ct_maketest(cpu_eor_indy_overflow),
        ct_maketest(cpu_lda_indy),
        ct_maketest(cpu_lda_indy_zero),
        ct_maketest(cpu_lda_indy_negative),
        ct_maketest(cpu_lda_indy_overflow),
        ct_maketest(cpu_ora_indy),
        ct_maketest(cpu_ora_indy_zero),
        ct_maketest(cpu_ora_indy_ones),
        ct_maketest(cpu_ora_indy_overflow),

        ct_maketest(cpu_and_abs),
        ct_maketest(cpu_and_abs_zero),
        ct_maketest(cpu_and_abs_ones),
        ct_maketest(cpu_eor_abs),
        ct_maketest(cpu_eor_abs_zero),
        ct_maketest(cpu_eor_abs_ones),
        ct_maketest(cpu_lda_abs),
        ct_maketest(cpu_lda_abs_zero),
        ct_maketest(cpu_lda_abs_negative),
        ct_maketest(cpu_ldx_abs),
        ct_maketest(cpu_ldx_abs_zero),
        ct_maketest(cpu_ldx_abs_negative),
        ct_maketest(cpu_ldy_abs),
        ct_maketest(cpu_ldy_abs_zero),
        ct_maketest(cpu_ldy_abs_negative),
        ct_maketest(cpu_ora_abs),
        ct_maketest(cpu_ora_abs_zero),
        ct_maketest(cpu_ora_abs_ones),

        ct_maketest(cpu_and_absx),
        ct_maketest(cpu_and_absx_zero),
        ct_maketest(cpu_and_absx_ones),
        ct_maketest(cpu_and_absx_overflow),
        ct_maketest(cpu_eor_absx),
        ct_maketest(cpu_eor_absx_zero),
        ct_maketest(cpu_eor_absx_ones),
        ct_maketest(cpu_eor_absx_overflow),
        ct_maketest(cpu_lda_absx),
        ct_maketest(cpu_lda_absx_zero),
        ct_maketest(cpu_lda_absx_negative),
        ct_maketest(cpu_lda_absx_overflow),
        ct_maketest(cpu_ldy_absx),
        ct_maketest(cpu_ldy_absx_zero),
        ct_maketest(cpu_ldy_absx_negative),
        ct_maketest(cpu_ldy_absx_overflow),
        ct_maketest(cpu_ora_absx),
        ct_maketest(cpu_ora_absx_zero),
        ct_maketest(cpu_ora_absx_ones),
        ct_maketest(cpu_ora_absx_overflow),

        ct_maketest(cpu_and_absy),
        ct_maketest(cpu_and_absy_zero),
        ct_maketest(cpu_and_absy_ones),
        ct_maketest(cpu_and_absy_overflow),
        ct_maketest(cpu_eor_absy),
        ct_maketest(cpu_eor_absy_zero),
        ct_maketest(cpu_eor_absy_ones),
        ct_maketest(cpu_eor_absy_overflow),
        ct_maketest(cpu_lda_absy),
        ct_maketest(cpu_lda_absy_zero),
        ct_maketest(cpu_lda_absy_negative),
        ct_maketest(cpu_lda_absy_overflow),
        ct_maketest(cpu_ldx_absy),
        ct_maketest(cpu_ldx_absy_zero),
        ct_maketest(cpu_ldx_absy_negative),
        ct_maketest(cpu_ldx_absy_overflow),
        ct_maketest(cpu_ora_absy),
        ct_maketest(cpu_ora_absy_zero),
        ct_maketest(cpu_ora_absy_ones),
        ct_maketest(cpu_ora_absy_overflow),

        ct_maketest(cpu_data_fault),
        ct_maketest(cpu_ram_mirroring),
    };

    return ct_makesuite(tests);
}
