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
    // NOTE: set the cpu ready to read instruction at 0x00
    cpu->pc = 0;
    cpu->signal.rdy = true;
}

static int clock_cpu(struct mos6502 *cpu)
{
    int cycles = 0;
    do {
        cycles += cpu_cycle(cpu);
        // NOTE: catch instructions that run longer than possible
        ct_asserttrue(cycles < 8);
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

//
// Zero-Page Instructions
//

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

//
// Absolute Instructions
//

static void cpu_lda_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x1, 0x80},
            abs[] = {0xff, 0x45};
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
    uint8_t mem[] = {0xad, 0x1, 0x80},
            abs[] = {0xff, 0x0};
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
    uint8_t mem[] = {0xad, 0x1, 0x80},
            abs[] = {0xff, 0x80};
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
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

        ct_maketest(cpu_lda_imm),
        ct_maketest(cpu_lda_imm_zero),
        ct_maketest(cpu_lda_imm_negative),

        ct_maketest(cpu_lda_zp),
        ct_maketest(cpu_lda_zp_zero),
        ct_maketest(cpu_lda_zp_negative),

        ct_maketest(cpu_lda_abs),
        ct_maketest(cpu_lda_abs_zero),
        ct_maketest(cpu_lda_abs_negative),
    };

    return ct_makesuite(tests);
}
