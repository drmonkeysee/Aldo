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

static void cpu_adc_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x6d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x6};
    cpu.a = 0xa;    // NOTE: 10 + 6
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

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

static void cpu_bit_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x2c, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x6};
    cpu.a = 0x2;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cmp_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xcd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x10};
    cpu.a = 0x10;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cpx_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xec, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x10};
    cpu.x = 0x10;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cpy_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xcc, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x10};
    cpu.y = 0x10;
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
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

static void cpu_sbc_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xed, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0x6};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.ram = mem;
    cpu.cart = abs;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sta_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {
        0x8d, 0x4, 0x2,
        [512] = 0x0, [513] = 0x0, [514] = 0x0, [515] = 0x0, [516] = 0x0,
    };
    cpu.a = 0xa;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[516]);
}

static void cpu_stx_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {
        0x8e, 0x4, 0x2,
        [512] = 0x0, [513] = 0x0, [514] = 0x0, [515] = 0x0, [516] = 0x0,
    };
    cpu.x = 0xf1;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf1u, mem[516]);
}

static void cpu_sty_abs(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {
        0x8c, 0x4, 0x2,
        [512] = 0x0, [513] = 0x0, [514] = 0x0, [515] = 0x0, [516] = 0x0,
    };
    cpu.y = 0x84;
    cpu.ram = mem;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x84u, mem[516]);
}

//
// Absolute,X Instructions
//

static void cpu_adc_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x7d, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.a = 0xa;    // NOTE: 10 + 6
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_adc_absx_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x7d, 0xff, 0x80};
    cpu.a = 0xa;    // NOTE: 10 + 178
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xbcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

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

static void cpu_and_absx_pagecross(void *ctx)
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

static void cpu_cmp_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xdd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.a = 0x10;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cmp_absx_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xdd, 0xff, 0x80};
    cpu.a = 0x10;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_eor_absx_pagecross(void *ctx)
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

static void cpu_lda_absx_pagecross(void *ctx)
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

static void cpu_ldy_absx_pagecross(void *ctx)
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

static void cpu_ora_absx_pagecross(void *ctx)
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

static void cpu_sbc_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xfd, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sbc_absx_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xfd, 0xff, 0x80};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - (-78)
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.x = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sta_absx(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {
        0x9d, 0x4, 0x2,
        [512] = 0x0, [513] = 0x0, [514] = 0x0, [515] = 0x0, [516] = 0x0,
        [517] = 0x0, [518] = 0x0, [519] = 0x0, [520] = 0x0, [521] = 0x0,
    };
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[519]);
}

static void cpu_sta_absx_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {
        0x9d, 0xff, 0x1,
        [512] = 0x0, [513] = 0x0, [514] = 0x0, [515] = 0x0, [516] = 0x0,
    };
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.x = 3;  // NOTE: cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[514]);
}

//
// Absolute,Y Instructions
//

static void cpu_adc_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x79, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.a = 0xa;    // NOTE: 10 + 6
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_adc_absy_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x79, 0xff, 0x80};
    cpu.a = 0xa;    // NOTE: 10 + 178
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xbcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

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

static void cpu_and_absy_pagecross(void *ctx)
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

static void cpu_cmp_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd9, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x10};
    cpu.a = 0x10;
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpu_cmp_absy_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xd9, 0xff, 0x80};
    cpu.a = 0x10;
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
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

static void cpu_eor_absy_pagecross(void *ctx)
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

static void cpu_lda_absy_pagecross(void *ctx)
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

static void cpu_ldx_absy_pagecross(void *ctx)
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

static void cpu_ora_absy_pagecross(void *ctx)
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

static void cpu_sbc_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xf9, 0x1, 0x80};
    const uint8_t abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.ram = mem;
    cpu.cart = abs;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sbc_absy_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xf9, 0xff, 0x80};
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - (-78)
    cpu.ram = mem;
    cpu.cart = bigrom;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cpu_sta_absy(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {
        0x99, 0x4, 0x2,
        [512] = 0x0, [513] = 0x0, [514] = 0x0, [515] = 0x0, [516] = 0x0,
        [517] = 0x0, [518] = 0x0, [519] = 0x0, [520] = 0x0, [521] = 0x0,
    };
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[519]);
}

static void cpu_sta_absy_pagecross(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {
        0x99, 0xff, 0x1,
        [512] = 0x0, [513] = 0x0, [514] = 0x0, [515] = 0x0, [516] = 0x0,
    };
    cpu.a = 0xa;
    cpu.ram = mem;
    cpu.y = 3;  // NOTE: cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[514]);
}

//
// Test List
//

struct ct_testsuite cpu_absolute_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(cpu_adc_abs),
        ct_maketest(cpu_and_abs),
        ct_maketest(cpu_bit_abs),
        ct_maketest(cpu_cmp_abs),
        ct_maketest(cpu_cpx_abs),
        ct_maketest(cpu_cpy_abs),
        ct_maketest(cpu_eor_abs),
        ct_maketest(cpu_lda_abs),
        ct_maketest(cpu_ldx_abs),
        ct_maketest(cpu_ldy_abs),
        ct_maketest(cpu_ora_abs),
        ct_maketest(cpu_sbc_abs),
        ct_maketest(cpu_sta_abs),
        ct_maketest(cpu_stx_abs),
        ct_maketest(cpu_sty_abs),

        ct_maketest(cpu_adc_absx),
        ct_maketest(cpu_adc_absx_pagecross),
        ct_maketest(cpu_and_absx),
        ct_maketest(cpu_and_absx_pagecross),
        ct_maketest(cpu_cmp_absx),
        ct_maketest(cpu_cmp_absx_pagecross),
        ct_maketest(cpu_eor_absx),
        ct_maketest(cpu_eor_absx_pagecross),
        ct_maketest(cpu_lda_absx),
        ct_maketest(cpu_lda_absx_pagecross),
        ct_maketest(cpu_ldy_absx),
        ct_maketest(cpu_ldy_absx_pagecross),
        ct_maketest(cpu_ora_absx),
        ct_maketest(cpu_ora_absx_pagecross),
        ct_maketest(cpu_sbc_absx),
        ct_maketest(cpu_sbc_absx_pagecross),
        ct_maketest(cpu_sta_absx),
        ct_maketest(cpu_sta_absx_pagecross),

        ct_maketest(cpu_adc_absy),
        ct_maketest(cpu_adc_absy_pagecross),
        ct_maketest(cpu_and_absy),
        ct_maketest(cpu_and_absy_pagecross),
        ct_maketest(cpu_cmp_absy),
        ct_maketest(cpu_cmp_absy_pagecross),
        ct_maketest(cpu_eor_absy),
        ct_maketest(cpu_eor_absy_pagecross),
        ct_maketest(cpu_lda_absy),
        ct_maketest(cpu_lda_absy_pagecross),
        ct_maketest(cpu_ldx_absy),
        ct_maketest(cpu_ldx_absy_pagecross),
        ct_maketest(cpu_ora_absy),
        ct_maketest(cpu_ora_absy_pagecross),
        ct_maketest(cpu_sbc_absy),
        ct_maketest(cpu_sbc_absy_pagecross),
        ct_maketest(cpu_sta_absy),
        ct_maketest(cpu_sta_absy_pagecross),
    };

    return ct_makesuite(tests);
}
