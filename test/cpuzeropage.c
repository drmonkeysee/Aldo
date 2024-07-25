//
//  cpuzeropage.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stddef.h>
#include <stdint.h>

//
// MARK: - Zero-Page Instructions
//

static void adc_zp(void *ctx)
{
    uint8_t mem[] = {0x65, 0x4, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void and_zp(void *ctx)
{
    uint8_t mem[] = {0x25, 0x4, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zp(void *ctx)
{
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x81};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zp_carryzero(void *ctx)
{
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zp_negative(void *ctx)
{
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void asl_zp_carrynegative(void *ctx)
{
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void asl_zp_all_ones(void *ctx)
{
    uint8_t mem[] = {0x6, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void bit_zp_maskset(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 2;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void bit_zp_maskclear(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 2;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void bit_zp_highmaskset(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x3f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x20;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x20u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void bit_zp_highmaskclear(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x1f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x20;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x20u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void bit_zp_sixmaskset(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x6f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x40;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void bit_zp_sixmaskclear(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x3f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x40;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void bit_zp_sevenmaskset(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xbf};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void bit_zp_sevenmaskclear(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0x3f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void bit_zp_set_high(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xc2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 2;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void bit_zp_zero_set_high(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xc2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void bit_zp_max_set_high(void *ctx)
{
    uint8_t mem[] = {0x24, 0x4, 0xff, 0xff, 0xc2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void cmp_zp(void *ctx)
{
    uint8_t mem[] = {0xc5, 0x4, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_zp(void *ctx)
{
    uint8_t mem[] = {0xe4, 0x4, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0x10;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_zp(void *ctx)
{
    uint8_t mem[] = {0xc4, 0x4, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x10;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void dec_zp(void *ctx)
{
    uint8_t mem[] = {0xc6, 0x4, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfu, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void dec_zp_zero(void *ctx)
{
    uint8_t mem[] = {0xc6, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void dec_zp_negative(void *ctx)
{
    uint8_t mem[] = {0xc6, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void eor_zp(void *ctx)
{
    uint8_t mem[] = {0x45, 0x4, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void inc_zp(void *ctx)
{
    uint8_t mem[] = {0xe6, 0x4, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void inc_zp_zero(void *ctx)
{
    uint8_t mem[] = {0xe6, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void inc_zp_negative(void *ctx)
{
    uint8_t mem[] = {0xe6, 0x4, 0xff, 0xff, 0x7f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[4]);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void lda_zp(void *ctx)
{
    uint8_t mem[] = {0xa5, 0x4, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldx_zp(void *ctx)
{
    uint8_t mem[] = {0xa6, 0x4, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldy_zp(void *ctx)
{
    uint8_t mem[] = {0xa4, 0x4, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zp(void *ctx)
{
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zp_carryzero(void *ctx)
{
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zp_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zp_all_ones(void *ctx)
{
    uint8_t mem[] = {0x46, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_zp(void *ctx)
{
    uint8_t mem[] = {0x5, 0x4, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_zp(void *ctx)
{
    uint8_t mem[] = {0x26, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x26, 0x4, 0xff, 0xff, 0x81};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x26, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_zp_carryzero(void *ctx)
{
    uint8_t mem[] = {0x26, 0x4, 0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_zp_negative(void *ctx)
{
    uint8_t mem[] = {0x26, 0x4, 0xff, 0xff, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x81u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rol_zp_carrynegative(void *ctx)
{
    uint8_t mem[] = {0x26, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rol_zp_all_ones(void *ctx)
{
    uint8_t mem[] = {0x26, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror_zp(void *ctx)
{
    uint8_t mem[] = {0x66, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x66, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x66, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_zp_carryzero(void *ctx)
{
    uint8_t mem[] = {0x66, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_zp_negative(void *ctx)
{
    uint8_t mem[] = {0x66, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[4]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror_zp_carrynegative(void *ctx)
{
    uint8_t mem[] = {0x66, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror_zp_all_ones(void *ctx)
{
    uint8_t mem[] = {0x66, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbc_zp(void *ctx)
{
    uint8_t mem[] = {0xe5, 0x4, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sta_zp(void *ctx)
{
    uint8_t mem[] = {0x85, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[4]);
}

static void stx_zp(void *ctx)
{
    uint8_t mem[] = {0x86, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xf1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf1u, mem[4]);
}

static void sty_zp(void *ctx)
{
    uint8_t mem[] = {0x84, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x84;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x84u, mem[4]);
}

//
// MARK: - Zero-Page,X Instructions
//

static void adc_zpx(void *ctx)
{
    uint8_t mem[] = {0x75, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 6
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x75, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 34
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2cu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void and_zpx(void *ctx)
{
    uint8_t mem[] = {0x35, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void and_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x35, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zpx(void *ctx)
{
    uint8_t mem[] = {0x16, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[7]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x16, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x44u, mem[2]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_zpx(void *ctx)
{
    uint8_t mem[] = {0xd5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xd5, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void dec_zpx(void *ctx)
{
    uint8_t mem[] = {0xd6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfu, mem[7]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void dec_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xd6, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x21u, mem[2]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_zpx(void *ctx)
{
    uint8_t mem[] = {0x55, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x55, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xd8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void inc_zpx(void *ctx)
{
    uint8_t mem[] = {0xf6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[7]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void inc_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xf6, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x23u, mem[2]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_zpx(void *ctx)
{
    uint8_t mem[] = {0xb5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x56u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xb5, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldy_zpx(void *ctx)
{
    uint8_t mem[] = {0xb4, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x56u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldy_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xb4, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zpx(void *ctx)
{
    uint8_t mem[] = {0x56, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[7]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x56, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[2]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_zpx(void *ctx)
{
    uint8_t mem[] = {0x15, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x15, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rol_zpx(void *ctx)
{
    uint8_t mem[] = {0x36, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[7]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x36, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x44u, mem[2]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_zpx(void *ctx)
{
    uint8_t mem[] = {0x76, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 4;
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[7]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x76, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[2]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbc_zpx(void *ctx)
{
    uint8_t mem[] = {0xf5, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xf5, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 8
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sta_zpx(void *ctx)
{
    uint8_t mem[] = {0x95, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[7]);
}

static void sta_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x95, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[2]);
}

static void sty_zpx(void *ctx)
{
    uint8_t mem[] = {0x94, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x84;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x84u, mem[7]);
}

static void sty_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x94, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x84;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x84u, mem[2]);
}

//
// MARK: - Zero-Page,Y Instructions
//

static void ldx_zpy(void *ctx)
{
    uint8_t mem[] = {0xb6, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x56u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldx_zpy_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xb6, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void stx_zpy(void *ctx)
{
    uint8_t mem[] = {0x96, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xf1;
    cpu.y = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf1u, mem[7]);
}

static void stx_zpy_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x96, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xf1;
    cpu.y = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf1u, mem[2]);
}

//
// MARK: - Unofficial Opcodes
//

static void dcp_zp_equal(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x10u, mem[4]);
}

static void dcp_zp_lt(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x41};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x40u, mem[4]);
}

static void dcp_zp_gt(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x7};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6u, mem[4]);
}

static void dcp_zp_max_to_min(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x0u, mem[4]);
}

static void dcp_zp_max_to_max(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xffu, mem[4]);
}

static void dcp_zp_min_to_max(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xffu, mem[4]);
}

static void dcp_zp_min_to_min(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x0u, mem[4]);
}

static void dcp_zp_neg_equal(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0xa1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xa0u, mem[4]);
}

static void dcp_zp_neg_lt(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0xffu, mem[4]);
}

static void dcp_zp_neg_gt(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x91};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x90u, mem[4]);
}

static void dcp_zp_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;    // Effectively -128 - 1 = 127

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: negative to positive always implies A > M
    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x1u, mem[4]);
}

static void dcp_zp_positive_to_negative(void *ctx)
{
    uint8_t mem[] = {0xc7, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;  // Effectively 0 - 1 = -1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: positive to negative always implies A < M
    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x1u, mem[4]);
}

static void dcp_zpx(void *ctx)
{
    uint8_t mem[] = {0xd7, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x10u, mem[7]);
}

static void dcp_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xd7, 0x3, 0x23, 0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x22u, mem[2]);
}

static void isc_zp(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6u, mem[4]);
}

static void isc_zp_borrowout(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 - 6 - B

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x3u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6u, mem[4]);
}

static void isc_zp_borrow(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0xfd};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;   // 10 - (-2)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xfeu, mem[4]);
}

static void isc_zp_zero(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x0u, mem[4]);
}

static void isc_zp_negative(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xff;    // -1 - 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x1u, mem[4]);
}

static void isc_zp_borrow_negative(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;  // 0 - 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x1u, mem[4]);
}

static void isc_zp_overflow_to_negative(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 - (-1)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0xffu, mem[4]);
}

static void isc_zp_overflow_to_positive(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;   // (-128) - 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x1u, mem[4]);
}

static void isc_zp_borrowout_causes_overflow(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;   // (-128) - 0 - B

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x0u, mem[4]);
}

static void isc_zp_borrowout_avoids_overflow(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x7f;   // 127 - (-1) - B

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xffu, mem[4]);
}

// SOURCE: nestest
static void isc_zp_overflow_without_borrow(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x7f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 - (-128)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x80u, mem[4]);
}

static void isc_bcd_zp(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 4;  // 4 - 2

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void isc_bcd_zp_digit_rollover(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x10;   // 10 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void isc_bcd_zp_not_supported(void *ctx)
{
    uint8_t mem[] = {0xe7, 0x4, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = false;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x10;   // 16 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void isc_zpx(void *ctx)
{
    uint8_t mem[] = {0xf7, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6u, mem[7]);
}

static void isc_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xf7, 0x3, 0x7, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 8
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x8u, mem[2]);
}

static void lax_zp(void *ctx)
{
    uint8_t mem[] = {0xa7, 0x4, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_zp_zero(void *ctx)
{
    uint8_t mem[] = {0xa7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_zp_negative(void *ctx)
{
    uint8_t mem[] = {0xa7, 0x4, 0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void lax_zpy(void *ctx)
{
    uint8_t mem[] = {0xb7, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x56u, cpu.a);
    ct_assertequal(0x56u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_zpy_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xb7, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0x56};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertequal(0x22u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void nop_zp(void *ctx)
{
    uint8_t nopcodes[] = {0x4, 0x44, 0x64};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x4, 0xff, 0xff, 0xc};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);

        int cycles = clock_cpu(&cpu);

        ct_assertequal(3, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);
        ct_assertequal(0xcu, cpu.databus, "Failed on opcode %02x", opc);

        // NOTE: verify NOP did nothing
        struct snapshot snp;
        cpu_snapshot(&cpu, &snp);
        ct_assertequal(0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.s, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.x, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.y, "Failed on opcode %02x", opc);
        ct_assertequal(0x34u, snp.cpu.status, "Failed on opcode %02x", opc);
    }
}

static void nop_zpx(void *ctx)
{
    uint8_t nopcodes[] = {0x14, 0x34, 0x54, 0x74, 0xd4, 0xf4};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xb};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.x = 4;

        int cycles = clock_cpu(&cpu);

        ct_assertequal(4, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);
        ct_assertequal(0xbu, cpu.databus, "Failed on opcode %02x", opc);

        // NOTE: verify NOP did nothing
        struct snapshot snp;
        cpu_snapshot(&cpu, &snp);
        ct_assertequal(0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.s, "Failed on opcode %02x", opc);
        ct_assertequal(4u, cpu.x, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.y, "Failed on opcode %02x", opc);
        ct_assertequal(0x34u, snp.cpu.status, "Failed on opcode %02x", opc);
    }
}

static void nop_zpx_pageoverflow(void *ctx)
{
    uint8_t nopcodes[] = {0x14, 0x34, 0x54, 0x74, 0xd4, 0xf4};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x3, 0x6, 0xff, 0xff, 0xff, 0xff, 0xb};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.x = 0xff;   // Wrap around from $0003 -> $0002

        int cycles = clock_cpu(&cpu);

        ct_assertequal(4, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);
        ct_assertequal(0x6u, cpu.databus, "Failed on opcode %02x", opc);

        // NOTE: verify NOP did nothing
        struct snapshot snp;
        cpu_snapshot(&cpu, &snp);
        ct_assertequal(0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.s, "Failed on opcode %02x", opc);
        ct_assertequal(0xffu, cpu.x, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.y, "Failed on opcode %02x", opc);
        ct_assertequal(0x34u, snp.cpu.status, "Failed on opcode %02x", opc);
    }
}

static void rla_zp(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 3;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0x81};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 5;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_zp_carryzero(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 5;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_zp_negative(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xff;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x81u, mem[4]);
    ct_assertequal(0x81u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rla_zp_carrynegative(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rla_zp_all_ones(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rla_zp_and_sets_zero_and_clears_negative(void *ctx)
{
    uint8_t mem[] = {0x27, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_zpx(void *ctx)
{
    uint8_t mem[] = {0x37, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.x = 4;
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[7]);
    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x37, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xf;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x44u, mem[2]);
    ct_assertequal(0x4u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rra_zp(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, mem[4]);
    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zp_carryin(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xd};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + (6 + C)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, mem[4]);
    ct_assertequal(0x11u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;   // (-1) + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, mem[4]);
    ct_assertequal(0x5u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zp_negative(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;  // 0 + (-1)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void rra_zp_carry_zero(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x81;   // (-127) + 127

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zp_carry_negative(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xff;   // (-1) + (-1)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void rra_zp_overflow_to_negative(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x7f;   // 127 + 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void rra_zp_overflow_to_positive(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;   // (-128) + (-1)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zp_carryin_causes_overflow(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x7f;   // 127 + (0 + C)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void rra_zp_carryin_avoids_overflow(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;   // (-128) + (-1 + C)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, mem[4]);
    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

// SOURCE: nestest
static void rra_zp_overflow_with_carry(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x7f;   // 127 + 127 + C

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void rra_bcd_zp(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 1;  // 1 + 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_bcd_zp_digit_rollover(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 9;  // 9 + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x15u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_bcd_zp_not_supported(void *ctx)
{
    uint8_t mem[] = {0x67, 0x4, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = false;
    cpu.p.d = true;
    cpu.a = 9;  // 9 + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zpx(void *ctx)
{
    uint8_t mem[] = {0x77, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 6
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, mem[7]);
    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void rra_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x77, 0x3, 0x44, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 34
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, mem[2]);
    ct_assertequal(0x2cu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sax_zp(void *ctx)
{
    uint8_t mem[] = {0x87, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa5;
    cpu.x = 0x3c;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(3, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x24u, mem[4]);
}

static void sax_zpy(void *ctx)
{
    uint8_t mem[] = {0x97, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa5;
    cpu.x = 0x3c;
    cpu.y = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x24u, mem[7]);
}

static void sax_zpy_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x97, 0x3, 0x8, 0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa5;
    cpu.x = 0x3c;
    cpu.y = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x24u, mem[2]);
}

static void slo_zp(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_assertequal(0x3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0x81};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[4]);
    ct_assertequal(0x3u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_zp_carryzero(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_zp_negative(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, mem[4]);
    ct_assertequal(0x81u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void slo_zp_carrynegative(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void slo_zp_all_ones(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, mem[4]);
    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void slo_zp_or_clears_zero_and_sets_negative(void *ctx)
{
    uint8_t mem[] = {0x7, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void slo_zpx(void *ctx)
{
    uint8_t mem[] = {0x17, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[7]);
    ct_assertequal(0x3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x17, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x44u, mem[2]);
    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zp(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zp_carry(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_assertequal(0x7cu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zp_zero(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zp_carryzero(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zp_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, mem[4]);
    ct_assertequal(0x43u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zp_all_ones(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, mem[4]);
    ct_assertequal(0x7cu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zp_eor_clears_zero_sets_negative(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, mem[4]);
    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sre_zp_eor_sets_zero(void *ctx)
{
    uint8_t mem[] = {0x47, 0x4, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[4]);
    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zpx(void *ctx)
{
    uint8_t mem[] = {0x57, 0x3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.x = 4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, mem[7]);
    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_zpx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x57, 0x3, 0x22, 0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.x = 0xff;   // Wrap around from $0003 -> $0002

    int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, mem[2]);
    ct_assertequal(0x12u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

//
// MARK: - Test List
//

struct ct_testsuite cpu_zeropage_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(adc_zp),
        ct_maketest(and_zp),
        ct_maketest(asl_zp),
        ct_maketest(asl_zp_carry),
        ct_maketest(asl_zp_zero),
        ct_maketest(asl_zp_carryzero),
        ct_maketest(asl_zp_negative),
        ct_maketest(asl_zp_carrynegative),
        ct_maketest(asl_zp_all_ones),
        ct_maketest(bit_zp_maskset),
        ct_maketest(bit_zp_maskclear),
        ct_maketest(bit_zp_highmaskset),
        ct_maketest(bit_zp_highmaskclear),
        ct_maketest(bit_zp_sixmaskset),
        ct_maketest(bit_zp_sixmaskclear),
        ct_maketest(bit_zp_sevenmaskset),
        ct_maketest(bit_zp_sevenmaskclear),
        ct_maketest(bit_zp_set_high),
        ct_maketest(bit_zp_zero_set_high),
        ct_maketest(bit_zp_max_set_high),
        ct_maketest(cmp_zp),
        ct_maketest(cpx_zp),
        ct_maketest(cpy_zp),
        ct_maketest(dec_zp),
        ct_maketest(dec_zp_zero),
        ct_maketest(dec_zp_negative),
        ct_maketest(eor_zp),
        ct_maketest(inc_zp),
        ct_maketest(inc_zp_zero),
        ct_maketest(inc_zp_negative),
        ct_maketest(lda_zp),
        ct_maketest(ldx_zp),
        ct_maketest(ldy_zp),
        ct_maketest(lsr_zp),
        ct_maketest(lsr_zp_carry),
        ct_maketest(lsr_zp_zero),
        ct_maketest(lsr_zp_carryzero),
        ct_maketest(lsr_zp_negative_to_positive),
        ct_maketest(lsr_zp_all_ones),
        ct_maketest(ora_zp),
        ct_maketest(rol_zp),
        ct_maketest(rol_zp_carry),
        ct_maketest(rol_zp_zero),
        ct_maketest(rol_zp_carryzero),
        ct_maketest(rol_zp_negative),
        ct_maketest(rol_zp_carrynegative),
        ct_maketest(rol_zp_all_ones),
        ct_maketest(ror_zp),
        ct_maketest(ror_zp_carry),
        ct_maketest(ror_zp_zero),
        ct_maketest(ror_zp_carryzero),
        ct_maketest(ror_zp_negative),
        ct_maketest(ror_zp_carrynegative),
        ct_maketest(ror_zp_all_ones),
        ct_maketest(sbc_zp),
        ct_maketest(sta_zp),
        ct_maketest(stx_zp),
        ct_maketest(sty_zp),

        ct_maketest(adc_zpx),
        ct_maketest(adc_zpx_pageoverflow),
        ct_maketest(and_zpx),
        ct_maketest(and_zpx_pageoverflow),
        ct_maketest(asl_zpx),
        ct_maketest(asl_zpx_pageoverflow),
        ct_maketest(cmp_zpx),
        ct_maketest(cmp_zpx_pageoverflow),
        ct_maketest(dec_zpx),
        ct_maketest(dec_zpx_pageoverflow),
        ct_maketest(eor_zpx),
        ct_maketest(eor_zpx_pageoverflow),
        ct_maketest(inc_zpx),
        ct_maketest(inc_zpx_pageoverflow),
        ct_maketest(lda_zpx),
        ct_maketest(lda_zpx_pageoverflow),
        ct_maketest(ldy_zpx),
        ct_maketest(ldy_zpx_pageoverflow),
        ct_maketest(lsr_zpx),
        ct_maketest(lsr_zpx_pageoverflow),
        ct_maketest(ora_zpx),
        ct_maketest(ora_zpx_pageoverflow),
        ct_maketest(rol_zpx),
        ct_maketest(rol_zpx_pageoverflow),
        ct_maketest(ror_zpx),
        ct_maketest(ror_zpx_pageoverflow),
        ct_maketest(sbc_zpx),
        ct_maketest(sbc_zpx_pageoverflow),
        ct_maketest(sta_zpx),
        ct_maketest(sta_zpx_pageoverflow),
        ct_maketest(sty_zpx),
        ct_maketest(sty_zpx_pageoverflow),

        ct_maketest(ldx_zpy),
        ct_maketest(ldx_zpy_pageoverflow),
        ct_maketest(stx_zpy),
        ct_maketest(stx_zpy_pageoverflow),

        // Unofficial Opcodes
        ct_maketest(dcp_zp_equal),
        ct_maketest(dcp_zp_lt),
        ct_maketest(dcp_zp_gt),
        ct_maketest(dcp_zp_max_to_min),
        ct_maketest(dcp_zp_max_to_max),
        ct_maketest(dcp_zp_min_to_max),
        ct_maketest(dcp_zp_min_to_min),
        ct_maketest(dcp_zp_neg_equal),
        ct_maketest(dcp_zp_neg_lt),
        ct_maketest(dcp_zp_neg_gt),
        ct_maketest(dcp_zp_negative_to_positive),
        ct_maketest(dcp_zp_positive_to_negative),
        ct_maketest(dcp_zpx),
        ct_maketest(dcp_zpx_pageoverflow),

        ct_maketest(isc_zp),
        ct_maketest(isc_zp_borrowout),
        ct_maketest(isc_zp_borrow),
        ct_maketest(isc_zp_zero),
        ct_maketest(isc_zp_negative),
        ct_maketest(isc_zp_borrow_negative),
        ct_maketest(isc_zp_overflow_to_negative),
        ct_maketest(isc_zp_overflow_to_positive),
        ct_maketest(isc_zp_borrowout_causes_overflow),
        ct_maketest(isc_zp_borrowout_avoids_overflow),
        ct_maketest(isc_zp_overflow_without_borrow),
        ct_maketest(isc_bcd_zp),
        ct_maketest(isc_bcd_zp_digit_rollover),
        ct_maketest(isc_bcd_zp_not_supported),
        ct_maketest(isc_zpx),
        ct_maketest(isc_zpx_pageoverflow),

        ct_maketest(lax_zp),
        ct_maketest(lax_zp_zero),
        ct_maketest(lax_zp_negative),
        ct_maketest(lax_zpy),
        ct_maketest(lax_zpy_pageoverflow),

        ct_maketest(nop_zp),
        ct_maketest(nop_zpx),
        ct_maketest(nop_zpx_pageoverflow),

        ct_maketest(rla_zp),
        ct_maketest(rla_zp_carry),
        ct_maketest(rla_zp_zero),
        ct_maketest(rla_zp_carryzero),
        ct_maketest(rla_zp_negative),
        ct_maketest(rla_zp_carrynegative),
        ct_maketest(rla_zp_all_ones),
        ct_maketest(rla_zp_and_sets_zero_and_clears_negative),
        ct_maketest(rla_zpx),
        ct_maketest(rla_zpx_pageoverflow),

        ct_maketest(rra_zp),
        ct_maketest(rra_zp_carryin),
        ct_maketest(rra_zp_carry),
        ct_maketest(rra_zp_zero),
        ct_maketest(rra_zp_negative),
        ct_maketest(rra_zp_carry_zero),
        ct_maketest(rra_zp_carry_negative),
        ct_maketest(rra_zp_overflow_to_negative),
        ct_maketest(rra_zp_overflow_to_positive),
        ct_maketest(rra_zp_carryin_causes_overflow),
        ct_maketest(rra_zp_carryin_avoids_overflow),
        ct_maketest(rra_zp_overflow_with_carry),
        ct_maketest(rra_bcd_zp),
        ct_maketest(rra_bcd_zp_digit_rollover),
        ct_maketest(rra_bcd_zp_not_supported),
        ct_maketest(rra_zpx),
        ct_maketest(rra_zpx_pageoverflow),

        ct_maketest(sax_zp),
        ct_maketest(sax_zpy),
        ct_maketest(sax_zpy_pageoverflow),

        ct_maketest(slo_zp),
        ct_maketest(slo_zp_carry),
        ct_maketest(slo_zp_zero),
        ct_maketest(slo_zp_carryzero),
        ct_maketest(slo_zp_negative),
        ct_maketest(slo_zp_carrynegative),
        ct_maketest(slo_zp_all_ones),
        ct_maketest(slo_zp_or_clears_zero_and_sets_negative),
        ct_maketest(slo_zpx),
        ct_maketest(slo_zpx_pageoverflow),

        ct_maketest(sre_zp),
        ct_maketest(sre_zp_carry),
        ct_maketest(sre_zp_zero),
        ct_maketest(sre_zp_carryzero),
        ct_maketest(sre_zp_negative_to_positive),
        ct_maketest(sre_zp_all_ones),
        ct_maketest(sre_zp_eor_clears_zero_sets_negative),
        ct_maketest(sre_zp_eor_sets_zero),
        ct_maketest(sre_zpx),
        ct_maketest(sre_zpx_pageoverflow),
    };

    return ct_makesuite(tests);
}
