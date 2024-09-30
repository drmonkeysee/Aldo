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
// MARK: - Immediate Instructions
//

static void adc(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;    // 10 + 6

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

// SOURCE: nestest
static void adc_overflow_does_not_include_carry(void *ctx)
{
    uint8_t mem[] = {0x69, 0x7f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x7f;   // 127 + 127

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);

    mem[1] = 0x80;
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x7f;   // 127 + (-128)

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);

    mem[1] = 0x7f;
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0x7f;   // 127 + 127 + C

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_bcd(void *ctx)
{
    uint8_t mem[] = {0x69, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 1;  // 1 + 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_digit_rollover(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 9;  // 9 + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x15u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_not_supported(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = false;
    cpu.p.d = true;
    cpu.a = 9;  // 9 + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_carryin(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.c = true;
    cpu.p.d = true;
    cpu.a = 9;  // 9 + (6 + C)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x16u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_carry(void *ctx)
{
    uint8_t mem[] = {0x69, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x98;   // 98 + 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_bcd_zero(void *ctx)
{
    uint8_t mem[] = {0x69, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0;  // 0 + 0

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_missed_zero(void *ctx)
{
    uint8_t mem[] = {0x69, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x99;   // 99 + 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_bcd_negative(void *ctx)
{
    uint8_t mem[] = {0x69, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x80;   // 80 + 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x81u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_bcd_overflow_to_negative(void *ctx)
{
    uint8_t mem[] = {0x69, 0x30};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x50;   // 50 + 30

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_bcd_overflow_to_positive(void *ctx)
{
    uint8_t mem[] = {0x69, 0x90};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x89;   // 89 + 90

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x79u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_carry_overflow(void *ctx)
{
    uint8_t mem[] = {0x69, 0x90};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x90;   // 90 + 90

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_carryin_causes_overflow(void *ctx)
{
    uint8_t mem[] = {0x69, 0x29};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.c = true;
    cpu.p.d = true;
    cpu.a = 0x50;   // 50 + 29 + C

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x80u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_bcd_overflow_does_not_include_carry(void *ctx)
{
    uint8_t mem[] = {0x69, 0x79};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x79;   // 79 + 79

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);

    mem[1] = 0x80;
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x79;   // 79 + 80

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x59u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);

    mem[1] = 0x79;
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0x79;   // 79 + 79 + C

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x59u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_bcd_max(void *ctx)
{
    uint8_t mem[] = {0x69, 0x99};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x99;   // 99 + 99

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x98u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_hex(void *ctx)
{
    uint8_t mem[] = {0x69, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0xa;    // 10 + 1 so far so good

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x11u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_high_hex(void *ctx)
{
    uint8_t mem[] = {0x69, 0xf};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0xf;    // 15 + 15 uh oh the lower digit adjustment acts strangely

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x14u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_bcd_max_hex(void *ctx)
{
    uint8_t mem[] = {0x69, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0xff;   // 30 + 30? 165 + 165??
                    // who knows, this is undocumented behavior

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x54u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

// SOURCE: http://visual6502.org/wiki/index.php?title=6502DecimalMode
static void adc_bcd_visual6502_cases(void *ctx)
{
    uint8_t mem[] = {0x69, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    uint8_t cases[][8] = {
        // A  +  M + C =  A   N  V  Z  C
        {0x00, 0x00, 0, 0x00, 0, 0, 1, 0},  // 0 + 0
        {0x79, 0x00, 1, 0x80, 1, 1, 0, 0},  // 79 + 0 + C
        {0x24, 0x56, 0, 0x80, 1, 1, 0, 0},  // 24 + 56
        {0x93, 0x82, 0, 0x75, 0, 1, 0, 1},  // 93 + 82
        {0x89, 0x76, 0, 0x65, 0, 0, 0, 1},  // 89 + 76
        {0x89, 0x76, 1, 0x66, 0, 0, 1, 1},  // 89 + 76 + C
        {0x80, 0xf0, 0, 0xd0, 0, 1, 0, 1},  // 80 + 150
        {0x80, 0xfa, 0, 0xe0, 1, 0, 0, 1},  // 80 + 160?
        {0x2f, 0x4f, 0, 0x74, 0, 0, 0, 0},  // 215? + 415?
        {0x6f, 0x00, 1, 0x76, 0, 0, 0, 0},  // 615? + 0 + C
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        const uint8_t *testcase = cases[i];
        cpu.pc = 0;
        cpu.a = testcase[0];
        mem[1] = testcase[1];
        cpu.p.c = testcase[2];

        int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on case %zu", i);
        ct_assertequal(2u, cpu.pc, "Failed on case %zu", i);

        ct_assertequal(testcase[3], cpu.a, "Failed on case %zu", i);
        ct_assertequal(testcase[4], cpu.p.n, "Failed on case %zu", i);
        ct_assertequal(testcase[5], cpu.p.v, "Failed on case %zu", i);
        ct_assertequal(testcase[6], cpu.p.z, "Failed on case %zu", i);
        ct_assertequal(testcase[7], cpu.p.c, "Failed on case %zu", i);
    }
}

static void and(void *ctx)
{
    uint8_t mem[] = {0x29, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

// SOURCE: nestest
static void sbc_overflow_does_not_include_borrow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0x80;   // (-128) - (-128)

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);

    mem[1] = 0x7f;
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0x80;   // (-128) - 127

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);

    mem[1] = 0x80;
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x80;   // (-128) - (-128) - B

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_bcd(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 4;  // 4 - 2

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_digit_rollover(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x10;   // 10 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_not_supported(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = false;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x10;   // 16 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_borrowout(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x10;   // 10 - 6 - B

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x3u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_borrow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x79};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x10;    // 10 - 79

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x31u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_bcd_zero(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 6;  // 6 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_negative(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x90;   // 90 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x84u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_bcd_borrow_negative(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0;  // 0 - 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x99u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_bcd_overflow_to_negative(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x90};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x79;   // 79 - 90

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x89u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_bcd_overflow_to_positive(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x80;   // 80 - 1

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x79u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_borrowout_causes_overflow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = false;
    cpu.a = 0x80;   // 80 - 0 - B

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x79u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_overflow_does_not_include_borrow(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0x80;   // 80 - 80

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);

    mem[1] = 0x79;
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0x80;   // 80 - 79

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);

    mem[1] = 0x80;
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x80;   // 80 - 80 - B

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x99u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_bcd_min(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x0;    // 0 - 0

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_max(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x99};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x99;   // 99 - 99

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_hex(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x1};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 + 1 so far so good

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x9u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_bcd_high_hex(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0xf;    // 15 - 10 = 90,15... 105?

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x9fu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void sbc_bcd_max_hex(void *ctx)
{
    uint8_t mem[] = {0xe9, 0x99};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0xff;   // 165? - 99 = 66.. this actually works

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x66u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);

    mem[1] = 0xff;
    cpu.pc = 0;
    cpu.a = 0x99;   // 99 - 165? = 34, we're in undocumented behavior territory

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x34u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

// SOURCE: http://visual6502.org/wiki/index.php?title=6502DecimalMode
static void sbc_bcd_visual6502_cases(void *ctx)
{
    uint8_t mem[] = {0xe9, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    uint8_t cases[][8] = {
        // A  -  M - B =  A   N  V  Z  C
        {0x00, 0x00, 0, 0x99, 1, 0, 0, 0},  // 0 - 0 - B
        {0x00, 0x00, 1, 0x00, 0, 0, 1, 1},  // 0 - 0
        {0x00, 0x01, 1, 0x99, 1, 0, 0, 0},  // 0 - 1
        {0x0a, 0x00, 1, 0x0a, 0, 0, 0, 1},  // 10 - 0
        {0x0b, 0x00, 0, 0x0a, 0, 0, 0, 1},  // 11 - 0 - B
        {0x9a, 0x00, 1, 0x9a, 1, 0, 0, 1},  // 100? - 0
        {0x9b, 0x00, 0, 0x9a, 1, 0, 0, 1},  // 101? - 0 - B
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        const uint8_t *testcase = cases[i];
        cpu.pc = 0;
        cpu.a = testcase[0];
        mem[1] = testcase[1];
        cpu.p.c = testcase[2];

        int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on case %zu", i);
        ct_assertequal(2u, cpu.pc, "Failed on case %zu", i);

        ct_assertequal(testcase[3], cpu.a, "Failed on case %zu", i);
        ct_assertequal(testcase[4], cpu.p.n, "Failed on case %zu", i);
        ct_assertequal(testcase[5], cpu.p.v, "Failed on case %zu", i);
        ct_assertequal(testcase[6], cpu.p.z, "Failed on case %zu", i);
        ct_assertequal(testcase[7], cpu.p.c, "Failed on case %zu", i);
    }
}

//
// MARK: - Unofficial Opcodes
//

static void alr(void *ctx)
{
    uint8_t mem[] = {0x4b, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa4;

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void anc(void *ctx)
{
    uint8_t nopcodes[] = {0x0b, 0x2b};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0xc};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.a = 0xa;

        int cycles = clock_cpu(&cpu);

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
    uint8_t nopcodes[] = {0x0b, 0x2b};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x0};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.a = 0xaa;

        int cycles = clock_cpu(&cpu);

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
    uint8_t nopcodes[] = {0x0b, 0x2b};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0xfc};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);
        cpu.a = 0xfa;

        int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);

        ct_assertequal(0xf8u, cpu.a, "Failed on opcode %02x", opc);
        ct_asserttrue(cpu.p.c, "Failed on opcode %02x", opc);
        ct_assertfalse(cpu.p.z, "Failed on opcode %02x", opc);
        ct_asserttrue(cpu.p.n, "Failed on opcode %02x", opc);
    }
}

static void ane(void *ctx)
{
    uint8_t mem[] = {0x8b, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x5a;
    cpu.x = 0x3f;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x3cu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ane_zero(void *ctx)
{
    uint8_t mem[] = {0x8b, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x5a;
    cpu.x = 0x3f;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ane_negative(void *ctx)
{
    uint8_t mem[] = {0x8b, 0xcf};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa5;
    cpu.x = 0xf3;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xc3u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void arr(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_zero(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_negative_exchanges_with_carry(void *ctx)
{
    uint8_t mem[] = {0x6b, 0xf6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xfa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x79u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_overflow(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x40};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x40;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x20u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_loses_carry(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x20;
    cpu.p.c = true;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x90u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void arr_negative_overflow_and_carry(void *ctx)
{
    uint8_t mem[] = {0x6b, 0xf0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;
    cpu.a = 0x80;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xc0u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void arr_bcd(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_bcd_low_adjustment(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0xf;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_bcd_low_adjustment_lost_carry(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x1f};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0x1f;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x5u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_bcd_high_adjustment(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x50};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.a = 0xf0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x88u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void arr_bcd_not_supported(void *ctx)
{
    uint8_t mem[] = {0x6b, 0x50};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = false;
    cpu.p.d = true;
    cpu.a = 0xf0;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x28u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void lxa(void *ctx)
{
    uint8_t mem[] = {0xab, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xcu, cpu.a);
    ct_assertequal(0xcu, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lxa_zero(void *ctx)
{
    uint8_t mem[] = {0xab, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x5a;
    cpu.x = 1;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertequal(0x0u, cpu.x);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lxa_negative(void *ctx)
{
    uint8_t mem[] = {0xab, 0xcf};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa4;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xceu, cpu.a);
    ct_assertequal(0xceu, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void nop(void *ctx)
{
    uint8_t nopcodes[] = {0x80, 0x82, 0x89, 0xc2, 0xe2};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x10};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, NULL);

        int cycles = clock_cpu(&cpu);

        ct_assertequal(2, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(2u, cpu.pc, "Failed on opcode %02x", opc);
        ct_assertequal(0x10u, cpu.databus, "Failed on opcode %02x", opc);

        // NOTE: verify NOP did nothing
        struct aldo_snapshot snp;
        cpu_snapshot(&cpu, &snp);
        ct_assertequal(0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.s, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.x, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.y, "Failed on opcode %02x", opc);
        ct_assertequal(0x34u, snp.cpu.status, "Failed on opcode %02x", opc);
    }
}

static void sbx_equal(void *ctx)
{
    uint8_t mem[] = {0xcb, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x11;
    cpu.x = 0x10;

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

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

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x7fu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

// SOURCE: nestest
static void usbc_overflow_does_not_include_borrow(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x7f;   // 127 - (-128) - B

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);

    mem[1] = 0x7f;
    cpu.p.c = false;
    cpu.pc = 0;
    cpu.a = 0x7f;   // 127 - 127 - B

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);

    mem[1] = 0x80;
    cpu.p.c = true;
    cpu.pc = 0;
    cpu.a = 0x7f;   // 127 - (-128)

    cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xffu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void usbc_bcd(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x2};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 4;  // 4 - 2

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_bcd_digit_rollover(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = true;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x10;   // 10 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void usbc_bcd_not_supported(void *ctx)
{
    uint8_t mem[] = {0xeb, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.bcd = false;
    cpu.p.d = true;
    cpu.p.c = true;
    cpu.a = 0x10;   // 16 - 6

    int cycles = clock_cpu(&cpu);

    ct_assertequal(2, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

//
// MARK: - Test List
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
        ct_maketest(adc_overflow_does_not_include_carry),
        ct_maketest(adc_bcd),
        ct_maketest(adc_bcd_digit_rollover),
        ct_maketest(adc_bcd_not_supported),
        ct_maketest(adc_bcd_carryin),
        ct_maketest(adc_bcd_carry),
        ct_maketest(adc_bcd_zero),
        ct_maketest(adc_bcd_missed_zero),
        ct_maketest(adc_bcd_negative),
        ct_maketest(adc_bcd_overflow_to_negative),
        ct_maketest(adc_bcd_overflow_to_positive),
        ct_maketest(adc_bcd_carry_overflow),
        ct_maketest(adc_bcd_carryin_causes_overflow),
        ct_maketest(adc_bcd_overflow_does_not_include_carry),
        ct_maketest(adc_bcd_max),
        ct_maketest(adc_bcd_hex),
        ct_maketest(adc_bcd_high_hex),
        ct_maketest(adc_bcd_max_hex),
        ct_maketest(adc_bcd_visual6502_cases),
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
        ct_maketest(sbc_overflow_does_not_include_borrow),
        ct_maketest(sbc_bcd),
        ct_maketest(sbc_bcd_digit_rollover),
        ct_maketest(sbc_bcd_not_supported),
        ct_maketest(sbc_bcd_borrowout),
        ct_maketest(sbc_bcd_borrow),
        ct_maketest(sbc_bcd_zero),
        ct_maketest(sbc_bcd_negative),
        ct_maketest(sbc_bcd_borrow_negative),
        ct_maketest(sbc_bcd_overflow_to_negative),
        ct_maketest(sbc_bcd_overflow_to_positive),
        ct_maketest(sbc_bcd_borrowout_causes_overflow),
        ct_maketest(sbc_bcd_overflow_does_not_include_borrow),
        ct_maketest(sbc_bcd_min),
        ct_maketest(sbc_bcd_max),
        ct_maketest(sbc_bcd_hex),
        ct_maketest(sbc_bcd_high_hex),
        ct_maketest(sbc_bcd_max_hex),
        ct_maketest(sbc_bcd_visual6502_cases),

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

        ct_maketest(ane),
        ct_maketest(ane_zero),
        ct_maketest(ane_negative),

        ct_maketest(arr),
        ct_maketest(arr_zero),
        ct_maketest(arr_negative_exchanges_with_carry),
        ct_maketest(arr_overflow),
        ct_maketest(arr_loses_carry),
        ct_maketest(arr_negative_overflow_and_carry),
        ct_maketest(arr_bcd),
        ct_maketest(arr_bcd_low_adjustment),
        ct_maketest(arr_bcd_low_adjustment_lost_carry),
        ct_maketest(arr_bcd_high_adjustment),
        ct_maketest(arr_bcd_not_supported),

        ct_maketest(lxa),
        ct_maketest(lxa_zero),
        ct_maketest(lxa_negative),

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
        ct_maketest(usbc_overflow_does_not_include_borrow),
        ct_maketest(usbc_bcd),
        ct_maketest(usbc_bcd_digit_rollover),
        ct_maketest(usbc_bcd_not_supported),
    };

    return ct_makesuite(tests);
}
