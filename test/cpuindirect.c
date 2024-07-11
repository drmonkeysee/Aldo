//
//  cpuindirect.c
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
// MARK: - (Indirect,X) Instructions
//

static void adc_indx(void *ctx)
{
    uint8_t mem[] = {0x61, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // 10 + 6
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
    cpu.a = 0xa;    // 10 + 34
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2cu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

static void adc_indx_zp_wraparound(void *ctx)
{
    uint8_t mem[] = {0x80, 0x61, 0x2, [255] = 0x2},
            abs[] = {0xff, 0x80, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.pc = 1;
    cpu.a = 0xa;    // 10 + 17
    cpu.x = 0xfd;   // Index to $00FF to fetch address $8002 across zp boundary

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x1bu, cpu.a);
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
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

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
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

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
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

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
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

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
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

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
    cpu.a = 0xa;    // 10 - 6
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
    cpu.a = 0xa;    // 10 - 6
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

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
    cpu.x = 0xff;   // Wrap around from $0008 -> $0007

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[10]);
}

//
// MARK: - (Indirect),Y Instructions
//

static void adc_indy(void *ctx)
{
    uint8_t mem[] = {0x71, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.a = 0xa;    // 10 + 6
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
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xa;    // 10 + 178
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xbcu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void adc_indy_zp_wraparound(void *ctx)
{
    uint8_t mem[] = {0x80, 0x71, 0xff, [255] = 0x1},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x8};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.pc = 1;
    cpu.a = 0xa;    // 10 + 8
    cpu.y = 3;  // Index to $00FF to fetch address $8001 + 3 across zp boundary

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(3u, cpu.pc);

    ct_assertequal(0x12u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
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
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

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
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0x10;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

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
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

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
    setup_cpu(&cpu, mem, BigRom);
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

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
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

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
    cpu.a = 0xa;    // 10 - 6
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
    setup_cpu(&cpu, mem, BigRom);
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - (-78)
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

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
    setup_cpu(&cpu, mem, BigRom);
    cpu.a = 0xa;
    cpu.y = 3;  // Cross boundary from $00FF -> $0102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, mem[258]);
}

//
// MARK: - Unofficial Opcodes
//

static void dcp_indx(void *ctx)
{
    uint8_t mem[] = {0xc3, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x10, RomWriteCapture);
}

static void dcp_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xc3, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x23};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x22, RomWriteCapture);
}

static void dcp_indy(void *ctx)
{
    uint8_t mem[] = {0xd3, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x11};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x10, RomWriteCapture);
}

static void dcp_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xd3, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0x10;
    cpu.y = 4;  // Cross boundary from $80FF -> $8103

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xb2, RomWriteCapture);
}

static void isc_indx(void *ctx)
{
    uint8_t mem[] = {0xe3, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void isc_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xe3, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void isc_indy(void *ctx)
{
    uint8_t mem[] = {0xf3, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x5};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - 6
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x4u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void isc_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xf3, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.p.c = true;
    cpu.a = 0xa;    // 10 - (-78)
    cpu.y = 2;  // Cross boundary from $80FF -> $8101

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x58u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xb2, RomWriteCapture);
}

static void lax_indx(void *ctx)
{
    uint8_t mem[] = {0xa3, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0xa3, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x22};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x22u, cpu.a);
    ct_assertequal(0x22u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_indy(void *ctx)
{
    uint8_t mem[] = {0xb3, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x45};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(5, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x45u, cpu.a);
    ct_assertequal(0x45u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
}

static void lax_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0xb3, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xb2u, cpu.a);
    ct_assertequal(0xb2u, cpu.x);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
}

static void rla_indx(void *ctx)
{
    uint8_t mem[] = {0x23, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;
    cpu.x = 4;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xd, RomWriteCapture);
}

static void rla_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x23, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xfa;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xf8u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0xfd, RomWriteCapture);
}

static void rla_indy(void *ctx)
{
    uint8_t mem[] = {0x33, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;
    cpu.y = 3;
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x8u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xd, RomWriteCapture);
}

static void rla_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x33, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102
    cpu.p.c = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x60u, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x65, RomWriteCapture);
}

static void rra_indx(void *ctx)
{
    uint8_t mem[] = {0x63, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 6
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void rra_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x63, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0x44};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 34
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2cu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x22, RomWriteCapture);
}

static void rra_indy(void *ctx)
{
    uint8_t mem[] = {0x73, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0xc};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 6
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x10u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x6, RomWriteCapture);
}

static void rra_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x73, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0xa;    // 10 + 178
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x63u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.v);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x59, RomWriteCapture);
}

static void sax_indx(void *ctx)
{
    uint8_t mem[] = {
        0x83, 0x2, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0,
        0xb, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 6;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x2u, mem[11]);
}

static void sax_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {
        0x83, 0x8, 0x0, 0xa, 0x0, 0xff, 0xff, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 7;
    cpu.x = 0xfb;   // Wrap around from $0008 -> $0003

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x3u, mem[10]);
}

static void sha_indy(void *ctx)
{
    uint8_t mem[] = {
        0x93, 0x2, 0x0, 0x1,
        [256] = 0xff, 0xff, 0xff, 0xee,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xa;
    cpu.x = 0xe;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xau, cpu.a);
    ct_assertequal(0xeu, cpu.x);
    ct_assertequal(0x2u, mem[259]);
}

static void sha_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x93, 0x2, 0xff, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0xea;
    cpu.x = 0xf8;
    cpu.y = 3;  // Cross boundary from $FEFF -> $FF02

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeau, cpu.a);
    ct_assertequal(0xf8u, cpu.x);
    ct_assertequal(0xe8u, cpu.databus);
    ct_assertequal(0xe802u, cpu.addrbus);
    ct_asserttrue(cpu.bflt);
}

static void slo_indx(void *ctx)
{
    uint8_t mem[] = {0x3, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xc, RomWriteCapture);
}

static void slo_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x3, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xfa;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xfeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0xfc, RomWriteCapture);
}

static void slo_indy(void *ctx)
{
    uint8_t mem[] = {0x13, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeu, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0xc, RomWriteCapture);
}

static void slo_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x13, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xeeu, cpu.a);
    ct_asserttrue(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x64, RomWriteCapture);
}

static void sre_indx(void *ctx)
{
    uint8_t mem[] = {0x43, 0x2, 0xff, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;
    cpu.x = 4;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x9u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x3, RomWriteCapture);
}

static void sre_indx_pageoverflow(void *ctx)
{
    uint8_t mem[] = {0x43, 0x2, 0x80, 0xff, 0xff, 0xff, 0x1, 0x80},
            abs[] = {0xff, 0x80, 0xfe};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xfa;
    cpu.x = 0xff;   // Wrap around from $0002 -> $0001

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x85u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x7f, RomWriteCapture);
}

static void sre_indy(void *ctx)
{
    uint8_t mem[] = {0x53, 0x2, 0x1, 0x80},
            abs[] = {0xff, 0xff, 0xff, 0xff, 0x6};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, abs);
    enable_rom_wcapture();
    cpu.a = 0xa;
    cpu.y = 3;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0x9u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_assertfalse(cpu.p.n);
    ct_assertequal(0x3, RomWriteCapture);
}

static void sre_indy_pagecross(void *ctx)
{
    uint8_t mem[] = {0x53, 0x2, 0xff, 0x80};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, BigRom);
    enable_rom_wcapture();
    cpu.a = 0xea;
    cpu.y = 3;  // Cross boundary from $80FF -> $8102

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(8, cycles);
    ct_assertequal(2u, cpu.pc);

    ct_assertequal(0xb3u, cpu.a);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.n);
    ct_assertequal(0x59, RomWriteCapture);
}

//
// MARK: - Test List
//

struct ct_testsuite cpu_indirect_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(adc_indx),
        ct_maketest(adc_indx_pageoverflow),
        ct_maketest(adc_indx_zp_wraparound),
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
        ct_maketest(adc_indy_zp_wraparound),
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

        // Unofficial Opcodes
        ct_maketest(dcp_indx),
        ct_maketest(dcp_indx_pageoverflow),
        ct_maketest(dcp_indy),
        ct_maketest(dcp_indy_pagecross),

        ct_maketest(isc_indx),
        ct_maketest(isc_indx_pageoverflow),
        ct_maketest(isc_indy),
        ct_maketest(isc_indy_pagecross),

        ct_maketest(lax_indx),
        ct_maketest(lax_indx_pageoverflow),
        ct_maketest(lax_indy),
        ct_maketest(lax_indy_pagecross),

        ct_maketest(rla_indx),
        ct_maketest(rla_indx_pageoverflow),
        ct_maketest(rla_indy),
        ct_maketest(rla_indy_pagecross),

        ct_maketest(rra_indx),
        ct_maketest(rra_indx_pageoverflow),
        ct_maketest(rra_indy),
        ct_maketest(rra_indy_pagecross),

        ct_maketest(sax_indx),
        ct_maketest(sax_indx_pageoverflow),

        ct_maketest(sha_indy),
        ct_maketest(sha_indy_pagecross),

        ct_maketest(slo_indx),
        ct_maketest(slo_indx_pageoverflow),
        ct_maketest(slo_indy),
        ct_maketest(slo_indy_pagecross),

        ct_maketest(sre_indx),
        ct_maketest(sre_indx_pageoverflow),
        ct_maketest(sre_indy),
        ct_maketest(sre_indy_pagecross),
    };

    return ct_makesuite(tests);
}
