//
//  cpuimmediate.c
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
// Immediate Instructions
//

static void adc(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_carryin(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 + (6 + C)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_carry(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;   // (-1) + 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x5u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_zero(void *ctx)
{
    uint8_t mem[] = {0x69, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_negative(void *ctx)
{
    uint8_t mem[] = {0x69, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;  // 0 + (-1)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_carry_zero(void *ctx)
{
    uint8_t mem[] = {0x69, 0x7f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x81;   // (-127) + 127

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_carry_negative(void *ctx)
{
    uint8_t mem[] = {0x69, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;   // (-1) + (-1)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_overflow_to_negative(void *ctx)
{
    uint8_t mem[] = {0x69, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x7f;   // 127 + 1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_overflow_to_positive(void *ctx)
{
    uint8_t mem[] = {0x69, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;   // (-128) + (-1)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_carryin_causes_overflow(void *ctx)
{
    uint8_t mem[] = {0x69, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 + (0 + C)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_carryin_avoids_overflow(void *ctx)
{
    uint8_t mem[] = {0x69, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;   // (-128) + (-1 + C)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

// SOURCE: nestest
static void adc_overflow_with_carry(void *ctx)
{
    uint8_t mem[] = {0x69, 0x7f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 + 127 + C

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void and(void *ctx)
{
    uint8_t mem[] = {0x29, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void and_zero(void *ctx)
{
    uint8_t mem[] = {0x29, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xaa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void and_negative(void *ctx)
{
    uint8_t mem[] = {0x29, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cmp_equal(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_lt(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cmp_gt(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_max_to_min(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cmp_max_to_max(void *ctx)
{
    uint8_t mem[] = {0xc9, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_min_to_max(void *ctx)
{
    uint8_t mem[] = {0xc9, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_min_to_min(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_neg_equal(void *ctx)
{
    uint8_t mem[] = {0xc9, 0xa0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_neg_lt(void *ctx)
{
    uint8_t mem[] = {0xc9, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cmp_neg_gt(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x90};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;    // Effectively -128 - 1 = 127

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: negative to positive always implies A > M
    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_positive_to_negative(void *ctx)
{
    uint8_t mem[] = {0xc9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;  // Effectively 0 - 1 = -1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: positive to negative always implies A < M
    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpx_equal(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_lt(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpx_gt(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_max_to_min(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpx_max_to_max(void *ctx)
{
    uint8_t mem[] = {0xe0, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_min_to_max(void *ctx)
{
    uint8_t mem[] = {0xe0, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_min_to_min(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_neg_equal(void *ctx)
{
    uint8_t mem[] = {0xe0, 0xa0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_neg_lt(void *ctx)
{
    uint8_t mem[] = {0xe0, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpx_neg_gt(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x90};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0x80;    // Effectively -128 - 1 = 127

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: negative to positive always implies X > M
    ct_assertequal(0x80u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_positive_to_negative(void *ctx)
{
    uint8_t mem[] = {0xe0, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0;  // Effectively 0 - 1 = -1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: positive to negative always implies X < M
    ct_assertequal(0x0u, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpy_equal(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_lt(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.y);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpy_gt(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_max_to_min(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpy_max_to_max(void *ctx)
{
    uint8_t mem[] = {0xc0, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_min_to_max(void *ctx)
{
    uint8_t mem[] = {0xc0, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_min_to_min(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_neg_equal(void *ctx)
{
    uint8_t mem[] = {0xc0, 0xa0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_neg_lt(void *ctx)
{
    uint8_t mem[] = {0xc0, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.y);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpy_neg_gt(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x90};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa0u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x80;    // Effectively -128 - 1 = 127

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: negative to positive always implies Y > M
    ct_assertequal(0x80u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_positive_to_negative(void *ctx)
{
    uint8_t mem[] = {0xc0, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0;  // Effectively 0 - 1 = -1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: positive to negative always implies Y < M
    ct_assertequal(0x0u, cpu.y);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void eor(void *ctx)
{
    uint8_t mem[] = {0x49, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_zero(void *ctx)
{
    uint8_t mem[] = {0x49, 0xaa};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xaa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_negative(void *ctx)
{
    uint8_t mem[] = {0x49, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void lda(void *ctx)
{
    uint8_t mem[] = {0xa9, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_zero(void *ctx)
{
    uint8_t mem[] = {0xa9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_negative(void *ctx)
{
    uint8_t mem[] = {0xa9, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ldx(void *ctx)
{
    uint8_t mem[] = {0xa2, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldx_zero(void *ctx)
{
    uint8_t mem[] = {0xa2, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldx_negative(void *ctx)
{
    uint8_t mem[] = {0xa2, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ldy(void *ctx)
{
    uint8_t mem[] = {0xa0, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldy_zero(void *ctx)
{
    uint8_t mem[] = {0xa0, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.y);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldy_negative(void *ctx)
{
    uint8_t mem[] = {0xa0, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ora(void *ctx)
{
    uint8_t mem[] = {0x9, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_zero(void *ctx)
{
    uint8_t mem[] = {0x9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_negative(void *ctx)
{
    uint8_t mem[] = {0x9, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x45;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbc(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_borrowout(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 - 6 - B

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x3u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_borrow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;   // 10 - (-2)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_zero(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_negative(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xff;    // -1 - 1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_borrow_negative(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;  // 0 - 1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_overflow_to_negative(void *ctx)
{
    uint8_t mem[] = {0xe9, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 - (-1)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_overflow_to_positive(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;   // (-128) - 1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_borrowout_causes_overflow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;   // (-128) - 0 - B

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_borrowout_avoids_overflow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x7f;   // 127 - (-1) - B

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

// SOURCE: nestest
static void sbc_overflow_without_borrow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 - (-128)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

//
// Unofficial Opcodes
//

static void alr(void *ctx)
{
    uint8_t mem[] = {0x4b, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void alr_carry(void *ctx)
{
    uint8_t mem[] = {0x4b, 0xf1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x78u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void alr_zero(void *ctx)
{
    uint8_t mem[] = {0x4b, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void alr_carryzero(void *ctx)
{
    uint8_t mem[] = {0x4b, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void alr_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0x4b, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x40u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void alr_all_ones(void *ctx)
{
    uint8_t mem[] = {0x4b, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void anc(void *ctx)
{
    const uint8_t nopcodes[] = {0x0b, 0x2b};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        const uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0xc};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.a = 0xa;

        const int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);

        ct_assertequal(0x8u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertfalse(cpu.p.c, "Failed on opcode %02x", opc);
        ct_assertfalse(cpu.p.z, "Failed on opcode %02x", opc);
        ct_assertfalse(cpu.p.n, "Failed on opcode %02x", opc);
    }
}

static void anc_zero(void *ctx)
{
    const uint8_t nopcodes[] = {0x0b, 0x2b};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        const uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x0};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.a = 0xaa;

        const int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);

        ct_assertequal(0x0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertfalse(cpu.p.c, "Failed on opcode %02x", opc);
        ct_asserttrue(cpu.p.z, "Failed on opcode %02x", opc);
        ct_assertfalse(cpu.p.n, "Failed on opcode %02x", opc);
    }
}

static void anc_negative(void *ctx)
{
    const uint8_t nopcodes[] = {0x0b, 0x2b};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        const uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0xfc};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.a = 0xfa;

        const int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);

        ct_assertequal(0xf8u, cpu.a, "Failed on opcode %02x", opc);
        ct_asserttrue(cpu.p.c, "Failed on opcode %02x", opc);
        ct_assertfalse(cpu.p.z, "Failed on opcode %02x", opc);
        ct_asserttrue(cpu.p.n, "Failed on opcode %02x", opc);
    }
}

static void nop(void *ctx)
{
    const uint8_t nopcodes[] = {0x80, 0x82, 0x89, 0xc2, 0xe2};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        const uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x10};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);

        const int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);
        ct_assertequal(0x10u, cpu.databus, "Failed on opcode %02x", opc);

        // NOTE: verify NOP did nothing
        struct console_state sn;
        cpu_snapshot(&cpu, &sn);
        ct_assertequal(0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.s, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.x, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.y, "Failed on opcode %02x", opc);
        ct_assertequal(0x34u, sn.cpu.status, "Failed on opcode %02x", opc);
    }
}

static void sbx_equal(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x11;
    cpu.x = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_lt(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x11;
    cpu.x = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xd0u, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbx_gt(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x11;
    cpu.x = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_max_to_min(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;
    cpu.x = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbx_max_to_max(void *ctx)
{
    uint8_t mem[] = {0xcb, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xff;
    cpu.x = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_min_to_max(void *ctx)
{
    uint8_t mem[] = {0xcb, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_min_to_min(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_neg_equal(void *ctx)
{
    uint8_t mem[] = {0xcb, 0xa0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xaf;
    cpu.x = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_neg_lt(void *ctx)
{
    uint8_t mem[] = {0xcb, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xaf;
    cpu.x = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa1u, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbx_neg_gt(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x90};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xaf;
    cpu.x = 0xa0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_negative_to_positive(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x8f;
    cpu.x = 0x80;    // Effectively -128 - 1 = 127

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: negative to positive always implies X > M
    ct_assertequal(0x7fu, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sbx_positive_to_negative(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 0;  // Effectively 0 - 1 = -1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    // NOTE: positive to negative always implies X < M
    ct_assertequal(0xffu, cpu.x);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void usbc(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_borrowout(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 - 6 - B

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x3u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_borrow(void *ctx)
{
    uint8_t mem[] = {0xeb, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xa;   // 10 - (-2)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_zero(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_negative(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0xff;    // -1 - 1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void usbc_borrow_negative(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0;  // 0 - 1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void usbc_overflow_to_negative(void *ctx)
{
    uint8_t mem[] = {0xeb, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 - (-1)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void usbc_overflow_to_positive(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;   // (-128) - 1

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_borrowout_causes_overflow(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x80;   // (-128) - 0 - B

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_borrowout_avoids_overflow(void *ctx)
{
    uint8_t mem[] = {0xeb, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x7f;   // 127 - (-1) - B

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

// SOURCE: nestest
static void usbc_overflow_without_borrow(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x7f;   // 127 - (-128)

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

//
// Test List
//

struct ct_testsuite cpu_immediate_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(adc),
        ct_maketest(adc_carryin),
        ct_maketest(adc_carry),
        ct_maketest(adc_zero),
        ct_maketest(adc_negative),
        ct_maketest(adc_carry_zero),
        ct_maketest(adc_carry_negative),
        ct_maketest(adc_overflow_to_negative),
        ct_maketest(adc_overflow_to_positive),
        ct_maketest(adc_carryin_causes_overflow),
        ct_maketest(adc_carryin_avoids_overflow),
        ct_maketest(adc_overflow_with_carry),
        ct_maketest(and),
        ct_maketest(and_zero),
        ct_maketest(and_negative),
        ct_maketest(cmp_equal),
        ct_maketest(cmp_lt),
        ct_maketest(cmp_gt),
        ct_maketest(cmp_max_to_min),
        ct_maketest(cmp_max_to_max),
        ct_maketest(cmp_min_to_max),
        ct_maketest(cmp_min_to_min),
        ct_maketest(cmp_neg_equal),
        ct_maketest(cmp_neg_lt),
        ct_maketest(cmp_neg_gt),
        ct_maketest(cmp_negative_to_positive),
        ct_maketest(cmp_positive_to_negative),
        ct_maketest(cpx_equal),
        ct_maketest(cpx_lt),
        ct_maketest(cpx_gt),
        ct_maketest(cpx_max_to_min),
        ct_maketest(cpx_max_to_max),
        ct_maketest(cpx_min_to_max),
        ct_maketest(cpx_min_to_min),
        ct_maketest(cpx_neg_equal),
        ct_maketest(cpx_neg_lt),
        ct_maketest(cpx_neg_gt),
        ct_maketest(cpx_negative_to_positive),
        ct_maketest(cpx_positive_to_negative),
        ct_maketest(cpy_equal),
        ct_maketest(cpy_lt),
        ct_maketest(cpy_gt),
        ct_maketest(cpy_max_to_min),
        ct_maketest(cpy_max_to_max),
        ct_maketest(cpy_min_to_max),
        ct_maketest(cpy_min_to_min),
        ct_maketest(cpy_neg_equal),
        ct_maketest(cpy_neg_lt),
        ct_maketest(cpy_neg_gt),
        ct_maketest(cpy_negative_to_positive),
        ct_maketest(cpy_positive_to_negative),
        ct_maketest(eor),
        ct_maketest(eor_zero),
        ct_maketest(eor_negative),
        ct_maketest(lda),
        ct_maketest(lda_zero),
        ct_maketest(lda_negative),
        ct_maketest(ldx),
        ct_maketest(ldx_zero),
        ct_maketest(ldx_negative),
        ct_maketest(ldy),
        ct_maketest(ldy_zero),
        ct_maketest(ldy_negative),
        ct_maketest(ora),
        ct_maketest(ora_zero),
        ct_maketest(ora_negative),
        ct_maketest(sbc),
        ct_maketest(sbc_borrowout),
        ct_maketest(sbc_borrow),
        ct_maketest(sbc_zero),
        ct_maketest(sbc_negative),
        ct_maketest(sbc_borrow_negative),
        ct_maketest(sbc_overflow_to_negative),
        ct_maketest(sbc_overflow_to_positive),
        ct_maketest(sbc_borrowout_causes_overflow),
        ct_maketest(sbc_borrowout_avoids_overflow),
        ct_maketest(sbc_overflow_without_borrow),

        // Unofficial Opcodes
        ct_maketest(alr),
        ct_maketest(alr_carry),
        ct_maketest(alr_zero),
        ct_maketest(alr_carryzero),
        ct_maketest(alr_negative_to_positive),
        ct_maketest(alr_all_ones),

        ct_maketest(anc),
        ct_maketest(anc_zero),
        ct_maketest(anc_negative),

        ct_maketest(nop),

        ct_maketest(sbx_equal),
        ct_maketest(sbx_lt),
        ct_maketest(sbx_gt),
        ct_maketest(sbx_max_to_min),
        ct_maketest(sbx_max_to_max),
        ct_maketest(sbx_min_to_max),
        ct_maketest(sbx_min_to_min),
        ct_maketest(sbx_neg_equal),
        ct_maketest(sbx_neg_lt),
        ct_maketest(sbx_neg_gt),
        ct_maketest(sbx_negative_to_positive),
        ct_maketest(sbx_positive_to_negative),

        ct_maketest(usbc),
        ct_maketest(usbc_borrowout),
        ct_maketest(usbc_borrow),
        ct_maketest(usbc_zero),
        ct_maketest(usbc_negative),
        ct_maketest(usbc_borrow_negative),
        ct_maketest(usbc_overflow_to_negative),
        ct_maketest(usbc_overflow_to_positive),
        ct_maketest(usbc_borrowout_causes_overflow),
        ct_maketest(usbc_borrowout_avoids_overflow),
        ct_maketest(usbc_overflow_without_borrow),
    };

    return ct_makesuite(tests);
}
