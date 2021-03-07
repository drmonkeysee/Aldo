//
//  cpuindirect.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"

#include <stdint.h>

//
// (Indirect,X) Instructions
//

static void cpu_and_indx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x21, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_and_indx_negative(void *ctx)
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

static void cpu_and_indx_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x21, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x80, 0x22};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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
    const uint8_t abs[] = {0xff, 0xaa};
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

static void cpu_eor_indx_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x41, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
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
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

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
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

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

static void cpu_ora_indx_negative(void *ctx)
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
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

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
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_and_indy_negative(void *ctx)
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

static void cpu_and_indy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x31, 0x2, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xaa};
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

static void cpu_eor_indy_negative(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x51, 0x2, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    cpu.a = 0xfa;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cpu_eor_indy_overflow(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x51, 0x2, 0xff, 0x80};
    cpu.a = 0xea;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

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
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

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

static void cpu_ora_indy_negative(void *ctx)
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
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

//
// Test List
//

struct ct_testsuite cpu_indirect_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_and_indx),
        ct_maketest(cpu_and_indx_zero),
        ct_maketest(cpu_and_indx_negative),
        ct_maketest(cpu_and_indx_overflow),
        ct_maketest(cpu_eor_indx),
        ct_maketest(cpu_eor_indx_zero),
        ct_maketest(cpu_eor_indx_negative),
        ct_maketest(cpu_eor_indx_overflow),
        ct_maketest(cpu_lda_indx),
        ct_maketest(cpu_lda_indx_zero),
        ct_maketest(cpu_lda_indx_negative),
        ct_maketest(cpu_lda_indx_overflow),
        ct_maketest(cpu_ora_indx),
        ct_maketest(cpu_ora_indx_zero),
        ct_maketest(cpu_ora_indx_negative),
        ct_maketest(cpu_ora_indx_overflow),

        ct_maketest(cpu_and_indy),
        ct_maketest(cpu_and_indy_zero),
        ct_maketest(cpu_and_indy_negative),
        ct_maketest(cpu_and_indy_overflow),
        ct_maketest(cpu_eor_indy),
        ct_maketest(cpu_eor_indy_zero),
        ct_maketest(cpu_eor_indy_negative),
        ct_maketest(cpu_eor_indy_overflow),
        ct_maketest(cpu_lda_indy),
        ct_maketest(cpu_lda_indy_zero),
        ct_maketest(cpu_lda_indy_negative),
        ct_maketest(cpu_lda_indy_overflow),
        ct_maketest(cpu_ora_indy),
        ct_maketest(cpu_ora_indy_zero),
        ct_maketest(cpu_ora_indy_negative),
        ct_maketest(cpu_ora_indy_overflow),
    };

    return ct_makesuite(tests);
}
