//
//  apu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "apu.h"
#include "ciny.h"
#include "cpuhelp.h"

#include <stddef.h>

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

static void synced_oam_sequence(void *ctx)
{
    // STA $4014    ; start OAM DMA
    // LDA #??      ; give the CPU something to do next
    // + 256 bytes at address $0200 to copy to OAM
    uint8_t mem[0x300] = {
        0x8d, 0x14, 0x40,
        0xa9, // $2004 will write here (mem[4]) due to cpuhelp write mask
    };
    struct aldo_rp2a03 apu;
    setup_apu(&apu, mem, nullptr);
    apu.cpu.a = 0x2;    // copy page $0200 to DMA
    // add values for OAM DMA, counting down from 0xff to 0x0
    for (auto i = 0; i < 256; ++i) {
        mem[0x200 + i] = (uint8_t)(255 - i);
    }
    auto cycles = 0;

    // setup write to $4014
    cycle_sync_apu(&apu);
    cycle_sync_apu(&apu);
    cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertfalse(apu.oam.active);
    ct_assertequal(0u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertequal(3u, apu.cpu.pc);
    ct_assertequal(0x8du, apu.cpu.opc);
    ct_assertequal(2u, apu.cpu.addrbus);
    ct_assertequal(0x40u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);

    // write 0x2 to $4014, starting OAM DMA sequence
    cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_asserttrue(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertequal(3u, apu.cpu.pc);
    ct_assertequal(0x8du, apu.cpu.opc);
    ct_assertequal(0x4014u, apu.cpu.addrbus);
    ct_assertequal(2u, apu.cpu.databus);
    ct_assertfalse(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);

    // DMA halts cpu, cpu executes read cycle
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_asserttrue(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(4u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);

    // DMA reads on get cycle, cpu idle
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_asserttrue(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(0x200u, apu.cpu.addrbus);
    ct_assertequal(0xffu, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(0u, mem[4]);

    // DMA writes on put cycle, cpu idle
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_asserttrue(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(0x2004u, apu.cpu.addrbus);
    ct_assertequal(0xffu, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(0xffu, mem[4]);

    // next DMA read
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_asserttrue(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(1u, apu.oam.low);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(0x201u, apu.cpu.addrbus);
    ct_assertequal(0xfeu, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(0xffu, mem[4]);

    // next write
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_asserttrue(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(1u, apu.oam.low);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(0x2004u, apu.cpu.addrbus);
    ct_assertequal(0xfeu, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(0xfeu, mem[4]);

    // run the rest of the DMA sequence
    do {
        cycles += cycle_sync_apu(&apu);
    } while (cycles < 513);

    // final DMA put cycle
    ct_assertequal(513, cycles);
    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_asserttrue(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(0xffu, apu.oam.low);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(0x2004u, apu.cpu.addrbus);
    ct_assertequal(0u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(0u, mem[4]);

    // DMA over, cpu resumes and reruns last read cycle
    cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertfalse(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(4u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);

    // next cpu cycle
    cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertfalse(apu.oam.active);
    ct_assertequal(2u, apu.oam.dma);
    ct_assertequal(0u, apu.oam.low);
    ct_assertequal(5u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(5u, apu.cpu.addrbus);
    ct_assertequal(0u, apu.cpu.databus);
    ct_assertequal(0u, apu.cpu.a);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
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

        ct_maketest(synced_oam_sequence),
    };

    return ct_makesuite(tests);
}
