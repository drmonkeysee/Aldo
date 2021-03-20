//
//  cpuzeropage.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"

#include <stdint.h>

//
// Zero-Page Instructions
//

static void cpu_adc_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x65, 0x4, 0xff, 0xff, 0x6};
    cpu.a = 0xa;    // NOTE: 10 + 6
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x25, 0x4, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_asl_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x1};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_asl_zp_carry(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x81};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_asl_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_asl_zp_carryzero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_asl_zp_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x40};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_asl_zp_carrynegative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_asl_zp_all_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0xff};
    cpu.ram = mem;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_bit_zp_maskset(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x6};
    cpu.a = 0x2;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_bit_zp_maskclear(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x5};
    cpu.a = 0x2;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_bit_zp_highmaskset(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x3f};
    cpu.a = 0x20;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x20u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_bit_zp_highmaskclear(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x1f};
    cpu.a = 0x20;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x20u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_bit_zp_sixmaskset(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x6f};
    cpu.a = 0x40;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_bit_zp_sixmaskclear(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x3f};
    cpu.a = 0x40;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_bit_zp_sevenmaskset(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xbf};
    cpu.a = 0x80;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void cpu_bit_zp_sevenmaskclear(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x3f};
    cpu.a = 0x80;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_bit_zp_set_high(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xc2};
    cpu.a = 0x2;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void cpu_bit_zp_zero_set_high(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xc2};
    cpu.a = 0x0;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void cpu_bit_zp_max_set_high(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xc2};
    cpu.a = 0xff;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void cpu_cmp_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc5, 0x4, 0xff, 0xff, 0x10};
    cpu.a = 0x10;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cpx_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe4, 0x4, 0xff, 0xff, 0x10};
    cpu.x = 0x10;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cpy_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc4, 0x4, 0xff, 0xff, 0x10};
    cpu.y = 0x10;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dec_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc6, 0x4, 0xff, 0xff, 0x10};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfu, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dec_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc6, 0x4, 0xff, 0xff, 0x1};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dec_zp_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xc6, 0x4, 0xff, 0xff, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
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

static void cpu_inc_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe6, 0x4, 0xff, 0xff, 0x10};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_inc_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe6, 0x4, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_inc_zp_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe6, 0x4, 0xff, 0xff, 0x7f};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
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

static void cpu_lsr_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x2};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lsr_zp_carry(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lsr_zp_zero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x0};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lsr_zp_carryzero(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x1};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lsr_zp_negative_to_positive(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x80};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lsr_zp_all_ones(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0xff};
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_sbc_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xe5, 0x4, 0xff, 0xff, 0x6};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sta_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x85, 0x4, 0xff, 0xff, 0x0};
    cpu.a = 0xa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[4]);
}

static void cpu_stx_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x86, 0x4, 0xff, 0xff, 0x0};
    cpu.x = 0xf1;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf1u, mem[4]);
}

static void cpu_sty_zp(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x84, 0x4, 0xff, 0xff, 0x0};
    cpu.y = 0x84;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x84u, mem[4]);
}

//
// Zero-Page,X Instructions
//

static void cpu_adc_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x75, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.a = 0xa;    // NOTE: 10 + 6
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_adc_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x75, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.a = 0xa;    // NOTE: 10 + 34
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2cu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x35, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_and_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x35, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_asl_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x16, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[7]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_asl_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x16, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x44u, mem[2]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cmp_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.a = 0x10;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cmp_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd5, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.a = 0x10;
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_dec_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfu, mem[7]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_dec_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd6, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x21u, mem[2]);
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

static void cpu_eor_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x55, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xd8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_inc_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xf6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[7]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_inc_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xf6, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x23u, mem[2]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_lda_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb5, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

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

static void cpu_ldy_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb4, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lsr_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x56, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2};
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[7]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_lsr_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x56, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[2]);
    ct_assertfalse(cpu.p.c);
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

static void cpu_ora_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x15, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_sbc_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xf5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sbc_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xf5, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 8
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sta_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x95, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[7]);
}

static void cpu_sta_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x95, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[2]);
}

static void cpu_sty_zpx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x94, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.y = 0x84;
    cpu.ram = mem;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x84u, mem[7]);
}

static void cpu_sty_zpx_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x94, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.y = 0x84;
    cpu.ram = mem;
    cpu.x = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x84u, mem[2]);
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

static void cpu_ldx_zpy_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xb6, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    cpu.ram = mem;
    cpu.y = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_stx_zpy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x96, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    cpu.x = 0xf1;
    cpu.ram = mem;
    cpu.y = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf1u, mem[7]);
}

static void cpu_stx_zpy_pageoverflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x96, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.x = 0xf1;
    cpu.ram = mem;
    cpu.y = 0xff;   // NOTE: wrap around from $0003 -> $0002

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf1u, mem[2]);
}

//
// Test List
//

struct ct_testsuite cpu_zeropage_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_adc_zp),
        ct_maketest(cpu_and_zp),
        ct_maketest(cpu_asl_zp),
        ct_maketest(cpu_asl_zp_carry),
        ct_maketest(cpu_asl_zp_zero),
        ct_maketest(cpu_asl_zp_carryzero),
        ct_maketest(cpu_asl_zp_negative),
        ct_maketest(cpu_asl_zp_carrynegative),
        ct_maketest(cpu_asl_zp_all_ones),
        ct_maketest(cpu_bit_zp_maskset),
        ct_maketest(cpu_bit_zp_maskclear),
        ct_maketest(cpu_bit_zp_highmaskset),
        ct_maketest(cpu_bit_zp_highmaskclear),
        ct_maketest(cpu_bit_zp_sixmaskset),
        ct_maketest(cpu_bit_zp_sixmaskclear),
        ct_maketest(cpu_bit_zp_sevenmaskset),
        ct_maketest(cpu_bit_zp_sevenmaskclear),
        ct_maketest(cpu_bit_zp_set_high),
        ct_maketest(cpu_bit_zp_zero_set_high),
        ct_maketest(cpu_bit_zp_max_set_high),
        ct_maketest(cpu_cmp_zp),
        ct_maketest(cpu_cpx_zp),
        ct_maketest(cpu_cpy_zp),
        ct_maketest(cpu_dec_zp),
        ct_maketest(cpu_dec_zp_zero),
        ct_maketest(cpu_dec_zp_negative),
        ct_maketest(cpu_eor_zp),
        ct_maketest(cpu_inc_zp),
        ct_maketest(cpu_inc_zp_zero),
        ct_maketest(cpu_inc_zp_negative),
        ct_maketest(cpu_lda_zp),
        ct_maketest(cpu_ldx_zp),
        ct_maketest(cpu_ldy_zp),
        ct_maketest(cpu_lsr_zp),
        ct_maketest(cpu_lsr_zp_carry),
        ct_maketest(cpu_lsr_zp_zero),
        ct_maketest(cpu_lsr_zp_carryzero),
        ct_maketest(cpu_lsr_zp_negative_to_positive),
        ct_maketest(cpu_lsr_zp_all_ones),
        ct_maketest(cpu_ora_zp),
        ct_maketest(cpu_sbc_zp),
        ct_maketest(cpu_sta_zp),
        ct_maketest(cpu_stx_zp),
        ct_maketest(cpu_sty_zp),

        ct_maketest(cpu_adc_zpx),
        ct_maketest(cpu_adc_zpx_pageoverflow),
        ct_maketest(cpu_and_zpx),
        ct_maketest(cpu_and_zpx_pageoverflow),
        ct_maketest(cpu_asl_zpx),
        ct_maketest(cpu_asl_zpx_pageoverflow),
        ct_maketest(cpu_cmp_zpx),
        ct_maketest(cpu_cmp_zpx_pageoverflow),
        ct_maketest(cpu_dec_zpx),
        ct_maketest(cpu_dec_zpx_pageoverflow),
        ct_maketest(cpu_eor_zpx),
        ct_maketest(cpu_eor_zpx_pageoverflow),
        ct_maketest(cpu_inc_zpx),
        ct_maketest(cpu_inc_zpx_pageoverflow),
        ct_maketest(cpu_lda_zpx),
        ct_maketest(cpu_lda_zpx_pageoverflow),
        ct_maketest(cpu_ldy_zpx),
        ct_maketest(cpu_ldy_zpx_pageoverflow),
        ct_maketest(cpu_lsr_zpx),
        ct_maketest(cpu_lsr_zpx_pageoverflow),
        ct_maketest(cpu_ora_zpx),
        ct_maketest(cpu_ora_zpx_pageoverflow),
        ct_maketest(cpu_sbc_zpx),
        ct_maketest(cpu_sbc_zpx_pageoverflow),
        ct_maketest(cpu_sta_zpx),
        ct_maketest(cpu_sta_zpx_pageoverflow),
        ct_maketest(cpu_sty_zpx),
        ct_maketest(cpu_sty_zpx_pageoverflow),

        ct_maketest(cpu_ldx_zpy),
        ct_maketest(cpu_ldx_zpy_pageoverflow),
        ct_maketest(cpu_stx_zpy),
        ct_maketest(cpu_stx_zpy_pageoverflow),
    };

    return ct_makesuite(tests);
}
