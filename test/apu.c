//
//  apu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "apu.h"
#include "ciny.h"
#include "cpuhelp.h"

static void powerup_initializes_apu(void *ctx)
{
    struct aldo_rp2a03 apu;

    aldo_apu_powerup(&apu);

    ct_assertequal(0u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertfalse(apu.oam.active);
    ct_asserttrue(apu.signal.rdy);
    ct_assertfalse(apu.put);
}

static void rst_detected_held_and_released(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct aldo_rp2a03 apu;
    setup_apu(&apu, mem, nullptr);
    apu.oam.dma = 0xa;
    apu.oam.low = 0xc;

    apu.cpu.signal.rst = false;
    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_DETECTED, (int)apu.cpu.rst);
    ct_assertequal(1u, apu.cpu.pc);
    ct_asserttrue(apu.put);
    ct_assertequal(0xau, apu.oam.dma);
    ct_assertequal(0xcu, apu.oam.low);

    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_PENDING, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_assertfalse(apu.put);
    ct_assertequal(0xau, apu.oam.dma);
    ct_assertequal(0xcu, apu.oam.low);

    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_assertfalse(apu.put);
    ct_assertequal(0xau, apu.oam.dma);
    ct_assertequal(0xcu, apu.oam.low);

    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_assertfalse(apu.put);
    ct_assertequal(0xau, apu.oam.dma);
    ct_assertequal(0xcu, apu.oam.low);

    apu.cpu.signal.rst = true;
    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_asserttrue(apu.put);
    ct_assertequal(0xau, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
}

static void rst_too_short(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct aldo_rp2a03 apu;
    setup_apu(&apu, mem, nullptr);

    apu.cpu.signal.rst = false;
    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_DETECTED, (int)apu.cpu.rst);
    ct_assertequal(1u, apu.cpu.pc);
    ct_asserttrue(apu.put);

    apu.cpu.signal.rst = true;
    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_assertfalse(apu.put);
}

//
// MARK: - OAM DMA
//

static void basic_oam_sequence(void *ctx)
{
    struct aldo_rp2a03 apu;

    ct_ignore("not implemented");
}

//
// MARK: - Test List
//

struct ct_testsuite apu_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_apu),
        ct_maketest(rst_detected_held_and_released),
        ct_maketest(rst_too_short),

        ct_maketest(basic_oam_sequence),
    };

    return ct_makesuite(tests);
}
