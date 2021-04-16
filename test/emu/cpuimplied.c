//
//  cpuimplied.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"
#include "emu/snapshot.h"

#include <stdint.h>

//
// Implied Instructions
//

static void asl(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa, 0xff};
    cpu.ram = mem;
    cpu.a = 1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_carry(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa, 0xff};
    cpu.ram = mem;
    cpu.a = 0x81;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa, 0xff};
    cpu.ram = mem;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_carryzero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa, 0xff};
    cpu.ram = mem;
    cpu.a = 0x80;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa, 0xff};
    cpu.ram = mem;
    cpu.a = 0x40;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void asl_carrynegative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void asl_all_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xa, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void clc(void *ctx)
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

static void cld(void *ctx)
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

static void cli(void *ctx)
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

static void clv(void *ctx)
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

static void dex(void *ctx)
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

static void dex_to_zero(void *ctx)
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

static void dex_to_negative(void *ctx)
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

static void dey(void *ctx)
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

static void dey_to_zero(void *ctx)
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

static void dey_to_negative(void *ctx)
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

static void inx(void *ctx)
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

static void inx_to_zero(void *ctx)
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

static void inx_to_negative(void *ctx)
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

static void iny(void *ctx)
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

static void iny_to_zero(void *ctx)
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

static void iny_to_negative(void *ctx)
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

static void lsr(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4a, 0xff};
    cpu.ram = mem;
    cpu.a = 2;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_carry(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4a, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4a, 0xff};
    cpu.ram = mem;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_carryzero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4a, 0xff};
    cpu.ram = mem;
    cpu.a = 1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_negative_to_positive(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4a, 0xff};
    cpu.ram = mem;
    cpu.a = 0x80;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x40u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_all_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4a, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void nop(void *ctx)
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

static void rol(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2a, 0xff};
    cpu.ram = mem;
    cpu.a = 0;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_carry(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2a, 0xff};
    cpu.ram = mem;
    cpu.a = 0x81;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2a, 0xff};
    cpu.ram = mem;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_carryzero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2a, 0xff};
    cpu.ram = mem;
    cpu.a = 0x80;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2a, 0xff};
    cpu.ram = mem;
    cpu.a = 0x40;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x81u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rol_carrynegative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2a, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rol_all_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2a, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6a, 0xff};
    cpu.ram = mem;
    cpu.a = 2;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_carry(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6a, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6a, 0xff};
    cpu.ram = mem;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_carryzero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6a, 0xff};
    cpu.ram = mem;
    cpu.a = 1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6a, 0xff};
    cpu.ram = mem;
    cpu.a = 0;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror_carrynegative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6a, 0xff};
    cpu.ram = mem;
    cpu.a = 1;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror_all_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6a, 0xff};
    cpu.ram = mem;
    cpu.a = 0xff;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(1u, cpu.pc);
    ct_assertequal(0xffu, cpu.databus);

    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sec(void *ctx)
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

static void sed(void *ctx)
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

static void sei(void *ctx)
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

static void tax(void *ctx)
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

static void tax_to_zero(void *ctx)
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

static void tax_to_negative(void *ctx)
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

static void tay(void *ctx)
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

static void tay_to_zero(void *ctx)
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

static void tay_to_negative(void *ctx)
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

static void tsx(void *ctx)
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

static void tsx_to_zero(void *ctx)
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

static void tsx_to_negative(void *ctx)
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

static void txa(void *ctx)
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

static void txa_to_zero(void *ctx)
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

static void txa_to_negative(void *ctx)
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

static void txs(void *ctx)
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

static void txs_to_zero(void *ctx)
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

static void txs_to_negative(void *ctx)
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

static void tya(void *ctx)
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

static void tya_to_zero(void *ctx)
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

static void tya_to_negative(void *ctx)
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
// Test List
//

struct ct_testsuite cpu_implied_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(asl),
        ct_maketest(asl_carry),
        ct_maketest(asl_zero),
        ct_maketest(asl_carryzero),
        ct_maketest(asl_negative),
        ct_maketest(asl_carrynegative),
        ct_maketest(asl_all_ones),
        ct_maketest(clc),
        ct_maketest(cld),
        ct_maketest(cli),
        ct_maketest(clv),
        ct_maketest(dex),
        ct_maketest(dex_to_zero),
        ct_maketest(dex_to_negative),
        ct_maketest(dey),
        ct_maketest(dey_to_zero),
        ct_maketest(dey_to_negative),
        ct_maketest(inx),
        ct_maketest(inx_to_zero),
        ct_maketest(inx_to_negative),
        ct_maketest(iny),
        ct_maketest(iny_to_zero),
        ct_maketest(iny_to_negative),
        ct_maketest(lsr),
        ct_maketest(lsr_carry),
        ct_maketest(lsr_zero),
        ct_maketest(lsr_carryzero),
        ct_maketest(lsr_negative_to_positive),
        ct_maketest(lsr_all_ones),
        ct_maketest(nop),
        ct_maketest(rol),
        ct_maketest(rol_carry),
        ct_maketest(rol_zero),
        ct_maketest(rol_carryzero),
        ct_maketest(rol_negative),
        ct_maketest(rol_carrynegative),
        ct_maketest(rol_all_ones),
        ct_maketest(ror),
        ct_maketest(ror_carry),
        ct_maketest(ror_zero),
        ct_maketest(ror_carryzero),
        ct_maketest(ror_negative),
        ct_maketest(ror_carrynegative),
        ct_maketest(ror_all_ones),
        ct_maketest(sec),
        ct_maketest(sed),
        ct_maketest(sei),
        ct_maketest(tax),
        ct_maketest(tax_to_zero),
        ct_maketest(tax_to_negative),
        ct_maketest(tay),
        ct_maketest(tay_to_zero),
        ct_maketest(tay_to_negative),
        ct_maketest(tsx),
        ct_maketest(tsx_to_zero),
        ct_maketest(tsx_to_negative),
        ct_maketest(txa),
        ct_maketest(txa_to_zero),
        ct_maketest(txa_to_negative),
        ct_maketest(txs),
        ct_maketest(txs_to_zero),
        ct_maketest(txs_to_negative),
        ct_maketest(tya),
        ct_maketest(tya_to_zero),
        ct_maketest(tya_to_negative),
    };

    return ct_makesuite(tests);
}
