//
//  cpuabsolute.c
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
// Absolute Instructions
//

static void adc_abs(void *ctx)
{
    uint8_t mem[] = {0x6d, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // 10 + 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void and_abs(void *ctx)
{
    uint8_t mem[] = {0x2d, 0x1, 0x80},
            abs[] = {0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_abs(void *ctx)
{
    uint8_t mem[] = {
        0xe, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x1,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[516]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void bit_abs(void *ctx)
{
    uint8_t mem[] = {0x2c, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 2;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void cmp_abs(void *ctx)
{
    uint8_t mem[] = {0xcd, 0x1, 0x80},
            abs[] = {0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpx_abs(void *ctx)
{
    uint8_t mem[] = {0xec, 0x1, 0x80},
            abs[] = {0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.x = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.x);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cpy_abs(void *ctx)
{
    uint8_t mem[] = {0xcc, 0x1, 0x80},
            abs[] = {0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.y = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.y);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void dec_abs(void *ctx)
{
    uint8_t mem[] = {
        0xce, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xffu, mem[516]);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void eor_abs(void *ctx)
{
    uint8_t mem[] = {0x4d, 0x1, 0x80},
            abs[] = {0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void inc_abs(void *ctx)
{
    uint8_t mem[] = {
        0xee, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[516]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_abs(void *ctx)
{
    uint8_t mem[] = {0xad, 0x1, 0x80},
            abs[] = {0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldx_abs(void *ctx)
{
    uint8_t mem[] = {0xae, 0x1, 0x80},
            abs[] = {0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldy_abs(void *ctx)
{
    uint8_t mem[] = {0xac, 0x1, 0x80},
            abs[] = {0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_abs(void *ctx)
{
    uint8_t mem[] = {
        0x4e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x2,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[516]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_abs(void *ctx)
{
    uint8_t mem[] = {0xd, 0x1, 0x80},
            abs[] = {0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_abs(void *ctx)
{
    uint8_t mem[] = {
        0x2e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[516]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_abs(void *ctx)
{
    uint8_t mem[] = {
        0x6e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, mem[516]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbc_abs(void *ctx)
{
    uint8_t mem[] = {0xed, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sta_abs(void *ctx)
{
    uint8_t mem[] = {
        0x8d, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[516]);
}

static void stx_abs(void *ctx)
{
    uint8_t mem[] = {
        0x8e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xf1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf1u, mem[516]);
}

static void sty_abs(void *ctx)
{
    uint8_t mem[] = {
        0x8c, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.y = 0x84;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x84u, mem[516]);
}

//
// Absolute,X Instructions
//

static void adc_absx(void *ctx)
{
    uint8_t mem[] = {0x7d, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // 10 + 6
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

static void adc_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0x7d, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xa;    // 10 + 178
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xbcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void and_absx(void *ctx)
{
    uint8_t mem[] = {0x3d, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void and_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0x3d, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xa2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void asl_absx(void *ctx)
{
    uint8_t mem[] = {
        0x1e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[519]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void asl_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x1e, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x1, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[514]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_absx(void *ctx)
{
    uint8_t mem[] = {0xdd, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0x10;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0xdd, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0x10;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void dec_absx(void *ctx)
{
    uint8_t mem[] = {
        0xde, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xffu, mem[519]);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void dec_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0xde, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xffu, mem[514]);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void eor_absx(void *ctx)
{
    uint8_t mem[] = {0x5d, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0x5d, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void inc_absx(void *ctx)
{
    uint8_t mem[] = {
        0xfe, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[519]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void inc_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0xfe, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[514]);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_absx(void *ctx)
{
    uint8_t mem[] = {0xbd, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0xbd, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ldy_absx(void *ctx)
{
    uint8_t mem[] = {0xbc, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldy_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0xbc, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.y);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void lsr_absx(void *ctx)
{
    uint8_t mem[] = {
        0x5e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[519]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lsr_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x5e, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x2, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[514]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_absx(void *ctx)
{
    uint8_t mem[] = {0x1d, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0x1d, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rol_absx(void *ctx)
{
    uint8_t mem[] = {
        0x3e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[519]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rol_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x3e, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;  // Cross boundary from $01FF -> $0202
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[514]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ror_absx(void *ctx)
{
    uint8_t mem[] = {
        0x7e, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, mem[519]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ror_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x7e, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;  // Cross boundary from $01FF -> $0202
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x80u, mem[514]);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbc_absx(void *ctx)
{
    uint8_t mem[] = {0xfd, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
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

static void sbc_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0xfd, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - (-78)
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sta_absx(void *ctx)
{
    uint8_t mem[] = {
        0x9d, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[519]);
}

static void sta_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x9d, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[514]);
}

//
// Absolute,Y Instructions
//

static void adc_absy(void *ctx)
{
    uint8_t mem[] = {0x79, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // 10 + 6
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

static void adc_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x79, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xa;    // 10 + 178
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xbcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void and_absy(void *ctx)
{
    uint8_t mem[] = {0x39, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void and_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x39, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xa2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cmp_absy(void *ctx)
{
    uint8_t mem[] = {0xd9, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0x10;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xd9, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0x10;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_absy(void *ctx)
{
    uint8_t mem[] = {0x59, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x59, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_absy(void *ctx)
{
    uint8_t mem[] = {0xb9, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xb9, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ldx_absy(void *ctx)
{
    uint8_t mem[] = {0xbe, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ldx_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xbe, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ora_absy(void *ctx)
{
    uint8_t mem[] = {0x19, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x19, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbc_absy(void *ctx)
{
    uint8_t mem[] = {0xf9, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
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

static void sbc_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xf9, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - (-78)
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sta_absy(void *ctx)
{
    uint8_t mem[] = {
        0x99, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[519]);
}

static void sta_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x99, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.y = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, mem[514]);
}

//
// Unofficial Opcodes
//

static void dcp_abs(void *ctx)
{
    uint8_t mem[] = {0xcf, 0x1, 0x80},
            abs[] = {0xff, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0x10;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x10, RomWriteCapture);
}

static void dcp_absx(void *ctx)
{
    uint8_t mem[] = {0xdf, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x10, RomWriteCapture);
}

static void dcp_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0xdf, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.x = 4;  // Cross boundary from $80FF -> $8103

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xb2, RomWriteCapture);
}

static void dcp_absy(void *ctx)
{
    uint8_t mem[] = {0xdb, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x10, RomWriteCapture);
}

static void dcp_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xdb, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.y = 4;  // Cross boundary from $80FF -> $8103

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xb2, RomWriteCapture);
}

static void isc_abs(void *ctx)
{
    uint8_t mem[] = {0xef, 0x1, 0x80},
            abs[] = {0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void isc_absx(void *ctx)
{
    uint8_t mem[] = {0xff, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void isc_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0xff, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - (-78)
    cpu.x = 2;  // Cross boundary from $80FF -> $8101

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xb2, RomWriteCapture);
}

static void isc_absy(void *ctx)
{
    uint8_t mem[] = {0xfb, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void isc_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xfb, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - (-78)
    cpu.y = 2;  // Cross boundary from $80FF -> $8101

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xb2, RomWriteCapture);
}

static void las_absy(void *ctx)
{
    uint8_t mem[] = {0xbb, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.s = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertequal(0x8u, cpu.x);
    ct_assertequal(0x8u, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void las_absy_zero(void *ctx)
{
    uint8_t mem[] = {0xbb, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.s = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x0u, cpu.a);
    ct_assertequal(0x0u, cpu.x);
    ct_assertequal(0x0u, cpu.s);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void las_absy_negative(void *ctx)
{
    uint8_t mem[] = {0xbb, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.s = 0xfa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_assertequal(0xf8u, cpu.x);
    ct_assertequal(0xf8u, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void las_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xbb, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.s = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xa2u, cpu.a);
    ct_assertequal(0xa2u, cpu.x);
    ct_assertequal(0xa2u, cpu.s);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void lax_abs(void *ctx)
{
    uint8_t mem[] = {0xaf, 0x1, 0x80},
            abs[] = {0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_absy(void *ctx)
{
    uint8_t mem[] = {0xbf, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xbf, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertequal(0xb2u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void nop_abs(void *ctx)
{
    uint8_t mem[] = {0xc, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(0x6u, cpu.databus);

    // NOTE: verify NOP did nothing
    struct console_state sn;
    cpu_snapshot(&cpu, &sn);
    ct_assertequal(0u, cpu.a);
    ct_assertequal(0u, cpu.s);
    ct_assertequal(0u, cpu.x);
    ct_assertequal(0u, cpu.y);
    ct_assertequal(0x34u, sn.cpu.status);
}

static void nop_absx(void *ctx)
{
    const uint8_t nopcodes[] = {0x1c, 0x3c, 0x5c, 0x7c, 0xdc, 0xfc};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        const uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0x1, 0x80},
                abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, abs);
        cpu.x = 3;

        const int cycles = clock_cpu(&cpu);

        ct_assertequal(4, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(3u, cpu.pc, "Failed on opcode %02x", opc);
        ct_assertequal(0x6u, cpu.databus, "Failed on opcode %02x", opc);

        // NOTE: verify NOP did nothing
        struct console_state sn;
        cpu_snapshot(&cpu, &sn);
        ct_assertequal(0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.s, "Failed on opcode %02x", opc);
        ct_assertequal(3u, cpu.x, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.y, "Failed on opcode %02x", opc);
        ct_assertequal(0x34u, sn.cpu.status, "Failed on opcode %02x", opc);
    }
}

static void nop_absx_pagecross(void *ctx)
{
    const uint8_t nopcodes[] = {0x1c, 0x3c, 0x5c, 0x7c, 0xdc, 0xfc};
    for (size_t c = 0; c < sizeof nopcodes / sizeof nopcodes[0]; ++c) {
        const uint8_t opc = nopcodes[c];
        uint8_t mem[] = {opc, 0xff, 0x80};
        struct mos6502 cpu;
        setup_cpu(&cpu, mem, BigRom);
        cpu.x = 3;  // Cross boundary from $80FF -> $8102

        const int cycles = clock_cpu(&cpu);

        ct_assertequal(5, cycles, "Failed on opcode %02x", opc);
        ct_assertequal(3u, cpu.pc, "Failed on opcode %02x", opc);
        ct_assertequal(0xb2u, cpu.databus, "Failed on opcode %02x", opc);

        // NOTE: verify NOP did nothing
        struct console_state sn;
        cpu_snapshot(&cpu, &sn);
        ct_assertequal(0u, cpu.a, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.s, "Failed on opcode %02x", opc);
        ct_assertequal(3u, cpu.x, "Failed on opcode %02x", opc);
        ct_assertequal(0u, cpu.y, "Failed on opcode %02x", opc);
        ct_assertequal(0x34u, sn.cpu.status, "Failed on opcode %02x", opc);
    }
}

static void rla_abs(void *ctx)
{
    uint8_t mem[] = {
        0x2f, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[516]);
    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_absx(void *ctx)
{
    uint8_t mem[] = {
        0x3f, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.x = 3;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[519]);
    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x3f, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.x = 3;  // Cross boundary from $01FF -> $0202
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[514]);
    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_absy(void *ctx)
{
    uint8_t mem[] = {
        0x3b, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.y = 3;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[519]);
    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rla_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x3b, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.y = 3;  // Cross boundary from $01FF -> $0202
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[514]);
    ct_assertequal(0x1u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void rra_abs(void *ctx)
{
    uint8_t mem[] = {0x6f, 0x1, 0x80},
            abs[] = {0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 6

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void rra_absx(void *ctx)
{
    uint8_t mem[] = {0x7f, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 6
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void rra_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0x7f, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 178
    cpu.x = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x63u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x59, RomWriteCapture);
}

static void rra_absy(void *ctx)
{
    uint8_t mem[] = {0x7b, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 6
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void rra_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x7b, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 178
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x63u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x59, RomWriteCapture);
}

static void sax_abs(void *ctx)
{
    uint8_t mem[] = {
        0x8f, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa5;
    cpu.x = 0x3c;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x24u, mem[516]);
}

static void sha_absy(void *ctx)
{
    uint8_t mem[] = {
        0x9f, 0x0, 0x1,
        [256] = 0xff, 0xff, 0xff, 0xee,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 0xe;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xau, cpu.a);
    ct_assertequal(0xeu, cpu.x);
    ct_assertequal(0x2u, mem[259]);
}

static void sha_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x9f, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xea;
    cpu.x = 0xf8;
    cpu.y = 3;  // Cross boundary from $FEFF -> $FF02

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeau, cpu.a);
    ct_assertequal(0xf8u, cpu.x);
    ct_assertequal(0xe8u, cpu.databus);
    ct_assertequal(0xe802u, cpu.addrbus);
    ct_asserttrue(cpu.bflt);
}

static void shx_absy(void *ctx)
{
    uint8_t mem[] = {
        0x9e, 0x0, 0x1,
        [256] = 0xff, 0xff, 0xff, 0xee,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xe;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.x);
    ct_assertequal(0x2u, mem[259]);
}

static void shx_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x9e, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 0xe1;
    cpu.y = 3;  // Cross boundary from $FEFF -> $FF02

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xe1u, cpu.x);
    ct_assertequal(0xe1u, cpu.databus);
    ct_assertequal(0xe102u, cpu.addrbus);
    ct_asserttrue(cpu.bflt);
}

static void shy_absx(void *ctx)
{
    uint8_t mem[] = {
        0x9c, 0x0, 0x1,
        [256] = 0xff, 0xff, 0xff, 0xee,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;
    cpu.y = 0xe;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeu, cpu.y);
    ct_assertequal(0x2u, mem[259]);
}

static void shy_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {0x9c, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.x = 3;  // Cross boundary from $FEFF -> $FF02
    cpu.y = 0xe1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xe1u, cpu.y);
    ct_assertequal(0xe1u, cpu.databus);
    ct_assertequal(0xe102u, cpu.addrbus);
    ct_asserttrue(cpu.bflt);
}

static void slo_abs(void *ctx)
{
    uint8_t mem[] = {
        0xf, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x1,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[516]);
    ct_assertequal(0x3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_absx(void *ctx)
{
    uint8_t mem[] = {
        0x1f, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[519]);
    ct_assertequal(0x3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x1f, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x1, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;
    cpu.x = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[514]);
    ct_assertequal(0x3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_absy(void *ctx)
{
    uint8_t mem[] = {
        0x1b, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[519]);
    ct_assertequal(0x3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void slo_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x1b, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x1, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 1;
    cpu.y = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x2u, mem[514]);
    ct_assertequal(0x3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_abs(void *ctx)
{
    uint8_t mem[] = {
        0x4f, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x2,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[516]);
    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_absx(void *ctx)
{
    uint8_t mem[] = {
        0x5f, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.x = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[519]);
    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_absx_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x5f, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x2, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.x = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[514]);
    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_absy(void *ctx)
{
    uint8_t mem[] = {
        0x5b, 0x4, 0x2,
        [512] = 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[519]);
    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void sre_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x5b, 0xff, 0x1,
        [512] = 0x0, 0x0, 0x2, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 3;
    cpu.y = 3;  // Cross boundary from $01FF -> $0202

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1u, mem[514]);
    ct_assertequal(0x2u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void tas_absy(void *ctx)
{
    uint8_t mem[] = {
        0x9b, 0x0, 0x1,
        [256] = 0xff, 0xff, 0xff, 0xee,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x5a;
    cpu.x = 0xee;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x5au, cpu.a);
    ct_assertequal(0xeeu, cpu.x);
    ct_assertequal(0x4au, cpu.s);
    ct_assertequal(0x2u, mem[259]);
}

static void tas_absy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x9b, 0xff, 0xce};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xea;
    cpu.x = 0xf8;
    cpu.y = 3;  // Cross boundary from $CEFF -> $CF02

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0xeau, cpu.a);
    ct_assertequal(0xf8u, cpu.x);
    ct_assertequal(0xe8u, cpu.s);
    ct_assertequal(0xc8u, cpu.databus);
    ct_assertequal(0xc802u, cpu.addrbus);
    ct_asserttrue(cpu.bflt);
}

//
// Test List
//

struct ct_testsuite cpu_absolute_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(adc_abs),
        ct_maketest(and_abs),
        ct_maketest(asl_abs),
        ct_maketest(bit_abs),
        ct_maketest(cmp_abs),
        ct_maketest(cpx_abs),
        ct_maketest(cpy_abs),
        ct_maketest(dec_abs),
        ct_maketest(eor_abs),
        ct_maketest(inc_abs),
        ct_maketest(lda_abs),
        ct_maketest(ldx_abs),
        ct_maketest(ldy_abs),
        ct_maketest(lsr_abs),
        ct_maketest(ora_abs),
        ct_maketest(rol_abs),
        ct_maketest(ror_abs),
        ct_maketest(sbc_abs),
        ct_maketest(sta_abs),
        ct_maketest(stx_abs),
        ct_maketest(sty_abs),

        ct_maketest(adc_absx),
        ct_maketest(adc_absx_pagecross),
        ct_maketest(and_absx),
        ct_maketest(and_absx_pagecross),
        ct_maketest(asl_absx),
        ct_maketest(asl_absx_pagecross),
        ct_maketest(cmp_absx),
        ct_maketest(cmp_absx_pagecross),
        ct_maketest(dec_absx),
        ct_maketest(dec_absx_pagecross),
        ct_maketest(eor_absx),
        ct_maketest(eor_absx_pagecross),
        ct_maketest(inc_absx),
        ct_maketest(inc_absx_pagecross),
        ct_maketest(lda_absx),
        ct_maketest(lda_absx_pagecross),
        ct_maketest(ldy_absx),
        ct_maketest(ldy_absx_pagecross),
        ct_maketest(lsr_absx),
        ct_maketest(lsr_absx_pagecross),
        ct_maketest(ora_absx),
        ct_maketest(ora_absx_pagecross),
        ct_maketest(rol_absx),
        ct_maketest(rol_absx_pagecross),
        ct_maketest(ror_absx),
        ct_maketest(ror_absx_pagecross),
        ct_maketest(sbc_absx),
        ct_maketest(sbc_absx_pagecross),
        ct_maketest(sta_absx),
        ct_maketest(sta_absx_pagecross),

        ct_maketest(adc_absy),
        ct_maketest(adc_absy_pagecross),
        ct_maketest(and_absy),
        ct_maketest(and_absy_pagecross),
        ct_maketest(cmp_absy),
        ct_maketest(cmp_absy_pagecross),
        ct_maketest(eor_absy),
        ct_maketest(eor_absy_pagecross),
        ct_maketest(lda_absy),
        ct_maketest(lda_absy_pagecross),
        ct_maketest(ldx_absy),
        ct_maketest(ldx_absy_pagecross),
        ct_maketest(ora_absy),
        ct_maketest(ora_absy_pagecross),
        ct_maketest(sbc_absy),
        ct_maketest(sbc_absy_pagecross),
        ct_maketest(sta_absy),
        ct_maketest(sta_absy_pagecross),

        // Unofficial Opcodes
        ct_maketest(dcp_abs),
        ct_maketest(dcp_absx),
        ct_maketest(dcp_absx_pagecross),
        ct_maketest(dcp_absy),
        ct_maketest(dcp_absy_pagecross),

        ct_maketest(isc_abs),
        ct_maketest(isc_absx),
        ct_maketest(isc_absx_pagecross),
        ct_maketest(isc_absy),
        ct_maketest(isc_absy_pagecross),

        ct_maketest(las_absy),
        ct_maketest(las_absy_zero),
        ct_maketest(las_absy_negative),
        ct_maketest(las_absy_pagecross),

        ct_maketest(lax_abs),
        ct_maketest(lax_absy),
        ct_maketest(lax_absy_pagecross),

        ct_maketest(nop_abs),
        ct_maketest(nop_absx),
        ct_maketest(nop_absx_pagecross),

        ct_maketest(rla_abs),
        ct_maketest(rla_absx),
        ct_maketest(rla_absx_pagecross),
        ct_maketest(rla_absy),
        ct_maketest(rla_absy_pagecross),

        ct_maketest(rra_abs),
        ct_maketest(rra_absx),
        ct_maketest(rra_absx_pagecross),
        ct_maketest(rra_absy),
        ct_maketest(rra_absy_pagecross),

        ct_maketest(sax_abs),

        ct_maketest(sha_absy),
        ct_maketest(sha_absy_pagecross),

        ct_maketest(shx_absy),
        ct_maketest(shx_absy_pagecross),

        ct_maketest(shy_absx),
        ct_maketest(shy_absx_pagecross),

        ct_maketest(slo_abs),
        ct_maketest(slo_absx),
        ct_maketest(slo_absx_pagecross),
        ct_maketest(slo_absy),
        ct_maketest(slo_absy_pagecross),

        ct_maketest(sre_abs),
        ct_maketest(sre_absx),
        ct_maketest(sre_absx_pagecross),
        ct_maketest(sre_absy),
        ct_maketest(sre_absy_pagecross),

        ct_maketest(tas_absy),
        ct_maketest(tas_absy_pagecross),
    };

    return ct_makesuite(tests);
}
