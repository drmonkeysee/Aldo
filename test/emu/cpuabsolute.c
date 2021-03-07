//
//  cpuabsolute.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"

#include <stdint.h>

//
// Absolute Instructions
//

static void cpu_and_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_and_abs_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2d,  0x1, 0x80};
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
    const uint8_t abs[] = {0xff, 0xaa};
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

static void cpu_eor_abs_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x4d,  0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
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

static void cpu_ora_abs_negative(void *ctx)
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
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_and_absx_negative(void *ctx)
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

static void cpu_and_absx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x3d, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xaa};
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

static void cpu_eor_absx_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_absx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x5d, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

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

static void cpu_ora_absx_negative(void *ctx)
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
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_and_absy_negative(void *ctx)
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

static void cpu_and_absy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x39, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xaa};
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

static void cpu_eor_absy_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x59, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_absy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x59, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

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

static void cpu_ora_absy_negative(void *ctx)
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
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Test List
//

struct ct_testsuite cpu_absolute_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_and_abs),
        ct_maketest(cpu_and_abs_zero),
        ct_maketest(cpu_and_abs_negative),
        ct_maketest(cpu_eor_abs),
        ct_maketest(cpu_eor_abs_zero),
        ct_maketest(cpu_eor_abs_negative),
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
        ct_maketest(cpu_ora_abs_negative),

        ct_maketest(cpu_and_absx),
        ct_maketest(cpu_and_absx_zero),
        ct_maketest(cpu_and_absx_negative),
        ct_maketest(cpu_and_absx_overflow),
        ct_maketest(cpu_eor_absx),
        ct_maketest(cpu_eor_absx_zero),
        ct_maketest(cpu_eor_absx_negative),
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
        ct_maketest(cpu_ora_absx_negative),
        ct_maketest(cpu_ora_absx_overflow),

        ct_maketest(cpu_and_absy),
        ct_maketest(cpu_and_absy_zero),
        ct_maketest(cpu_and_absy_negative),
        ct_maketest(cpu_and_absy_overflow),
        ct_maketest(cpu_eor_absy),
        ct_maketest(cpu_eor_absy_zero),
        ct_maketest(cpu_eor_absy_negative),
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
        ct_maketest(cpu_ora_absy_negative),
        ct_maketest(cpu_ora_absy_overflow),
    };

    return ct_makesuite(tests);
}
