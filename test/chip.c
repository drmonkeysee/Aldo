//
//  chip.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "chip.h"
#include "ciny.h"
#include "cpuhelp.h"

static void powerup_initializes_chip(void *ctx)
{
    struct aldo_rp2a03 chip;

    aldo_chip_powerup(&chip);

    ct_assertequal(0u, chip.oam.dma);
    ct_assertequal(0u, chip.oam.low);
    ct_assertfalse(chip.oam.active);
    ct_asserttrue(chip.signal.rdy);
    ct_assertfalse(chip.put);
}

static void rst_detected_held_and_released(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct aldo_rp2a03 chip;
    setup_chip(&chip, mem, nullptr);
    chip.oam.dma = 0xa;
    chip.oam.low = 0xc;

    chip.cpu.signal.rst = false;
    aldo_chip_cycle(&chip);

    ct_assertequal(ALDO_SIG_DETECTED, (int)chip.cpu.rst);
    ct_assertequal(1u, chip.cpu.pc);
    ct_asserttrue(chip.put);
    ct_assertequal(0xau, chip.oam.dma);
    ct_assertequal(0xcu, chip.oam.low);

    aldo_chip_cycle(&chip);

    ct_assertequal(ALDO_SIG_PENDING, (int)chip.cpu.rst);
    ct_assertequal(2u, chip.cpu.pc);
    ct_assertfalse(chip.put);
    ct_assertequal(0xau, chip.oam.dma);
    ct_assertequal(0xcu, chip.oam.low);

    aldo_chip_cycle(&chip);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)chip.cpu.rst);
    ct_assertequal(2u, chip.cpu.pc);
    ct_assertfalse(chip.put);
    ct_assertequal(0xau, chip.oam.dma);
    ct_assertequal(0xcu, chip.oam.low);

    aldo_chip_cycle(&chip);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)chip.cpu.rst);
    ct_assertequal(2u, chip.cpu.pc);
    ct_assertfalse(chip.put);
    ct_assertequal(0xau, chip.oam.dma);
    ct_assertequal(0xcu, chip.oam.low);

    chip.cpu.signal.rst = true;
    aldo_chip_cycle(&chip);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)chip.cpu.rst);
    ct_assertequal(2u, chip.cpu.pc);
    ct_asserttrue(chip.put);
    ct_assertequal(0xau, chip.oam.dma);
    ct_assertequal(0u, chip.oam.low);
}

static void rst_too_short(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct aldo_rp2a03 chip;
    setup_chip(&chip, mem, nullptr);

    chip.cpu.signal.rst = false;
    aldo_chip_cycle(&chip);

    ct_assertequal(ALDO_SIG_DETECTED, (int)chip.cpu.rst);
    ct_assertequal(1u, chip.cpu.pc);
    ct_asserttrue(chip.put);

    chip.cpu.signal.rst = true;
    aldo_chip_cycle(&chip);

    ct_assertequal(ALDO_SIG_CLEAR, (int)chip.cpu.rst);
    ct_assertequal(2u, chip.cpu.pc);
    ct_assertfalse(chip.put);
}

//
// MARK: - Test List
//

struct ct_testsuite chip_tests()
{
    static constexpr struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_chip),
        ct_maketest(rst_detected_held_and_released),
        ct_maketest(rst_too_short),
    };

    return ct_makesuite(tests);
}
