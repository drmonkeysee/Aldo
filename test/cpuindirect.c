//
//  cpuindirect.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stdint.h>

//
// (Indirect,X) Instructions
//

static void adc_indx(void *ctx)
{
    uint8_t mem[] = {0x61, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // NOTE: 10 + 6
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x61, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x22};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // NOTE: 10 + 34
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2cu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void and_indx(void *ctx)
{
    uint8_t mem[] = {0x21, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void and_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x21, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x22};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_indx(void *ctx)
{
    uint8_t mem[] = {0xc1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0x10;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xc1, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x22};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0x10;
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void eor_indx(void *ctx)
{
    uint8_t mem[] = {0x41, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x41, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_indx(void *ctx)
{
    uint8_t mem[] = {0xa1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xa1, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x22};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_indx(void *ctx)
{
    uint8_t mem[] = {0x1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x1, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbc_indx(void *ctx)
{
    uint8_t mem[] = {0xe1, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xe1, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.x = 0xff;   // NOTE: wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sta_indx(void *ctx)
{
    uint8_t mem[] = {
        0x81, 0x2, 0xff, 0xff, 0xff, 0xff, 0xb, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[11]);
}

static void sta_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {
        0x81, 0x8, 0x0, 0xff, 0xff, 0xff, 0xff, 0xa,
        0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 0xff;   // NOTE: wrap around from $0008 -> $0007

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[10]);
}

//
// (Indirect),Y Instructions
//

static void adc_indy(void *ctx)
{
    uint8_t mem[] = {0x71, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // NOTE: 10 + 6
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x71, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.a = 0xa;    // NOTE: 10 + 178
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xbcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void and_indy(void *ctx)
{
    uint8_t mem[] = {0x31, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void and_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x31, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.a = 0xea;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xa2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void cmp_indy(void *ctx)
{
    uint8_t mem[] = {0xd1, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x10};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0x10;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void cmp_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xd1, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.a = 0x10;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_indy(void *ctx)
{
    uint8_t mem[] = {0x51, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xfa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x6u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void eor_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x51, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.a = 0xea;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_indy(void *ctx)
{
    uint8_t mem[] = {0xb1, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lda_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xb1, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void ora_indy(void *ctx)
{
    uint8_t mem[] = {0x11, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void ora_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x11, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.a = 0xea;
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfau, cpu.a);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void sbc_indy(void *ctx)
{
    uint8_t mem[] = {0xf1, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - 6
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sbc_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xf1, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.p.c = true;
    cpu.a = 0xa;    // NOTE: 10 - (-78)
    cpu.y = 3;  // NOTE: cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void sta_indy(void *ctx)
{
    uint8_t mem[] = {
        0x91, 0x2, 0x4, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[7]);
}

static void sta_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {
        0x91, 0x2, 0xff, 0x0,
        [256] = 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, bigrom);
    cpu.a = 0xa;
    cpu.y = 3;  // NOTE: cross boundary from $00FF -> $0102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[258]);
}

//
// Test List
//

struct ct_testsuite cpu_indirect_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(adc_indx),
        ct_maketest(adc_indx_pageoverflow),
        ct_maketest(and_indx),
        ct_maketest(and_indx_pageoverflow),
        ct_maketest(cmp_indx),
        ct_maketest(cmp_indx_pageoverflow),
        ct_maketest(eor_indx),
        ct_maketest(eor_indx_pageoverflow),
        ct_maketest(lda_indx),
        ct_maketest(lda_indx_pageoverflow),
        ct_maketest(ora_indx),
        ct_maketest(ora_indx_pageoverflow),
        ct_maketest(sbc_indx),
        ct_maketest(sbc_indx_pageoverflow),
        ct_maketest(sta_indx),
        ct_maketest(sta_indx_pageoverflow),

        ct_maketest(adc_indy),
        ct_maketest(adc_indy_pagecross),
        ct_maketest(and_indy),
        ct_maketest(and_indy_pagecross),
        ct_maketest(cmp_indy),
        ct_maketest(cmp_indy_pagecross),
        ct_maketest(eor_indy),
        ct_maketest(eor_indy_pagecross),
        ct_maketest(lda_indy),
        ct_maketest(lda_indy_pagecross),
        ct_maketest(ora_indy),
        ct_maketest(ora_indy_pagecross),
        ct_maketest(sbc_indy),
        ct_maketest(sbc_indy_pagecross),
        ct_maketest(sta_indy),
        ct_maketest(sta_indy_pagecross),
    };

    return ct_makesuite(tests);
}
