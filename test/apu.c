//
//  apu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/22/26.
//

#include "apu.h"
#include "bus.h"
#include "bytes.h"
#include "ciny.h"
#include "cpuhelp.h"
#include "ctrlsignal.h"

#include <stdint.h>

struct readctx {
    struct aldo_busdevice inner;
    int count;
};

static bool count_cpu_reads(void *restrict ctx, uint16_t addr,
                            uint8_t *restrict d)
{
    struct readctx *rc = ctx;
    auto result = rc->inner.read(rc->inner.ctx, addr, d);
    // only count CPU reads, which the tests keep within zero page
    if (result && addr < 0x100) {
        ++rc->count;
    }
    return result;
}

static bool wrap_write(void *ctx, uint16_t addr, uint8_t d)
{
    const struct readctx *rc = ctx;
    return rc->inner.write(rc->inner.ctx, addr, d);
}

static void record_cpu_reads(struct readctx *ctx, aldo_bus *bus)
{
    *ctx = (typeof(*ctx)){};
    aldo_bus_swap(bus, 0, (struct aldo_busdevice){
        .read = count_cpu_reads,
        .write = wrap_write,
        .ctx = ctx}, &ctx->inner);
}

static bool oamwrite(void *ctx, uint16_t addr, uint8_t d)
{
    ct_assertequal(0x2004u, addr);
    uint8_t *oamdata = ctx;
    *oamdata = d;
    return true;
}

//
// MARK: - Startup
//

static void powerup_initializes_apu(void *ctx)
{
    struct aldo_rp2a03 apu;

    setup_apu(&apu, nullptr, nullptr);

    ct_assertequal(0u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.oam.s);
    ct_asserttrue(apu.signal.rdy);
    ct_assertfalse(apu.put);
}

static void rst_detected_held_and_released(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct aldo_rp2a03 apu;
    setup_apu(&apu, mem, nullptr);
    apu.oam.hi = 0xa;
    apu.oam.lo = 0xc;

    apu.cpu.signal.rst = false;
    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_DETECTED, (int)apu.cpu.rst);
    ct_assertequal(1u, apu.cpu.pc);
    ct_asserttrue(apu.put);
    ct_assertequal(0xau, apu.oam.hi);
    ct_assertequal(0xcu, apu.oam.lo);

    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_PENDING, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_assertfalse(apu.put);
    ct_assertequal(0xau, apu.oam.hi);
    ct_assertequal(0xcu, apu.oam.lo);

    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_assertfalse(apu.put);
    ct_assertequal(0xau, apu.oam.hi);
    ct_assertequal(0xcu, apu.oam.lo);

    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_assertfalse(apu.put);
    ct_assertequal(0xau, apu.oam.hi);
    ct_assertequal(0xcu, apu.oam.lo);

    apu.cpu.signal.rst = true;
    aldo_apu_cycle(&apu);

    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.cpu.rst);
    ct_assertequal(2u, apu.cpu.pc);
    ct_asserttrue(apu.put);
    ct_assertequal(0xau, apu.oam.hi);
    ct_assertequal(0xcu, apu.oam.lo);
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

static void aligned_oam_sequence(void *ctx)
{
    // STA $4014    ; start OAM DMA
    // LDA #$BB     ; give the CPU something to do next
    // + 256 bytes at address $0200 to copy to OAM
    uint8_t mem[0x300] = {
        0x8d, 0x14, 0x40,
        0xa9, 0xbb,
    };
    struct aldo_rp2a03 apu;
    setup_apu(&apu, mem, nullptr);
    // mock PPU bus device for OAMDATA
    uint8_t oamdata = 0;
    aldo_bus_set(apu.cpu.mbus, ALDO_MEMBLOCK_8KB,
                 (struct aldo_busdevice){.write = oamwrite, .ctx = &oamdata});
    struct readctx rc;
    record_cpu_reads(&rc, apu.cpu.mbus);
    // set put high to avoid dma sync cycle for this test
    apu.put = true;
    apu.oam.lo = 0xff;
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

    ct_assertfalse(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.oam.s);
    ct_assertequal(0u, apu.oam.hi);
    ct_assertequal(0xffu, apu.oam.lo);
    ct_assertequal(3u, apu.cpu.pc);
    ct_assertequal(0x8du, apu.cpu.opc);
    ct_assertequal(2u, apu.cpu.addrbus);
    ct_assertequal(0x40u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(3, rc.count);

    // write 0x2 to $4014, starting OAM DMA sequence
    cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(3u, apu.cpu.pc);
    ct_assertequal(0x8du, apu.cpu.opc);
    ct_assertequal(0x4014u, apu.cpu.addrbus);
    ct_assertequal(2u, apu.cpu.databus);
    ct_assertfalse(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(3, rc.count);

    // DMA halts cpu, cpu executes read cycle
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);

    // DMA reads on get cycle, cpu idle
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x200u, apu.addrbus);
    ct_assertequal(0xffu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);
    ct_assertequal(0u, oamdata);

    // DMA writes on put cycle, cpu idle
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(1u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0xffu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);
    ct_assertequal(0xffu, oamdata);

    // next DMA read
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(1u, apu.oam.lo);
    ct_assertequal(0x201u, apu.addrbus);
    ct_assertequal(0xfeu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);
    ct_assertequal(0xffu, oamdata);

    // next write
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(2u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0xfeu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);
    ct_assertequal(0xfeu, oamdata);

    // run the rest of the DMA sequence
    do {
        cycles += cycle_sync_apu(&apu);
    } while (cycles < 512);

    ct_assertequal(512, cycles);
    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0xffu, apu.oam.lo);
    ct_assertequal(0x2ffu, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);
    ct_assertequal(1u, oamdata);

    // final DMA put cycle
    cycles += cycle_sync_apu(&apu);

    ct_assertequal(513, cycles);
    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_SERVICED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);
    ct_assertequal(0u, oamdata);

    // DMA over, cpu resumes and reruns last read cycle
    cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);

    // next cpu cycle
    cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(5u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(4u, apu.cpu.addrbus);
    ct_assertequal(0xbbu, apu.cpu.databus);
    ct_assertequal(0xbbu, apu.cpu.a);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(6, rc.count);
}

static void unaligned_oam_sequence(void *ctx)
{
    // STA $4014    ; start OAM DMA
    // LDA #$BB     ; give the CPU something to do next
    // + 256 bytes at address $0200 to copy to OAM
    uint8_t mem[0x300] = {
        0x8d, 0x14, 0x40,
        0xa9, 0xbb,
    };
    struct aldo_rp2a03 apu;
    setup_apu(&apu, mem, nullptr);
    // mock PPU bus device for OAMDATA
    uint8_t oamdata = 0;
    aldo_bus_set(apu.cpu.mbus, ALDO_MEMBLOCK_8KB,
                 (struct aldo_busdevice){.write = oamwrite, .ctx = &oamdata});
    struct readctx rc;
    record_cpu_reads(&rc, apu.cpu.mbus);
    apu.oam.lo = 0xff;
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
    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.oam.s);
    ct_assertequal(0u, apu.oam.hi);
    ct_assertequal(0xffu, apu.oam.lo);
    ct_assertequal(3u, apu.cpu.pc);
    ct_assertequal(0x8du, apu.cpu.opc);
    ct_assertequal(2u, apu.cpu.addrbus);
    ct_assertequal(0x40u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(3, rc.count);

    // write 0x2 to $4014, starting OAM DMA sequence
    cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(3u, apu.cpu.pc);
    ct_assertequal(0x8du, apu.cpu.opc);
    ct_assertequal(0x4014u, apu.cpu.addrbus);
    ct_assertequal(2u, apu.cpu.databus);
    ct_assertfalse(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(3, rc.count);

    // DMA halts cpu, cpu executes read cycle
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(4, rc.count);

    // DMA alignment cycle, cpu reads again
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);

    // DMA reads on get cycle, cpu idle
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x200u, apu.addrbus);
    ct_assertequal(0xffu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);
    ct_assertequal(0u, oamdata);

    // DMA writes on put cycle, cpu idle
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(1u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0xffu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);
    ct_assertequal(0xffu, oamdata);

    // next DMA read
    cycles += cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(1u, apu.oam.lo);
    ct_assertequal(0x201u, apu.addrbus);
    ct_assertequal(0xfeu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);
    ct_assertequal(0xffu, oamdata);

    // next write
    cycles += cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_PENDING, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(2u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0xfeu, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);
    ct_assertequal(0xfeu, oamdata);

    // run the rest of the DMA sequence
    do {
        cycles += cycle_sync_apu(&apu);
    } while (cycles < 513);

    ct_assertequal(513, cycles);
    ct_asserttrue(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_COMMITTED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0xffu, apu.oam.lo);
    ct_assertequal(0x2ffu, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);
    ct_assertequal(1u, oamdata);

    // final DMA put cycle
    cycles += cycle_sync_apu(&apu);

    ct_assertequal(514, cycles);
    ct_assertfalse(apu.put);
    ct_assertfalse(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_SERVICED, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_assertfalse(apu.cpu.signal.rdy);
    ct_assertequal(5, rc.count);
    ct_assertequal(0u, oamdata);

    // DMA over, cpu resumes and reruns last read cycle
    cycle_sync_apu(&apu);

    ct_asserttrue(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(4u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(3u, apu.cpu.addrbus);
    ct_assertequal(0xa9u, apu.cpu.databus);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(6, rc.count);

    // next cpu cycle
    cycle_sync_apu(&apu);

    ct_assertfalse(apu.put);
    ct_asserttrue(apu.signal.rdy);
    ct_assertequal(ALDO_SIG_CLEAR, (int)apu.oam.s);
    ct_assertequal(2u, apu.oam.hi);
    ct_assertequal(0u, apu.oam.lo);
    ct_assertequal(0x2004u, apu.addrbus);
    ct_assertequal(0u, apu.databus);
    ct_assertequal(5u, apu.cpu.pc);
    ct_assertequal(0xa9u, apu.cpu.opc);
    ct_assertequal(4u, apu.cpu.addrbus);
    ct_assertequal(0xbbu, apu.cpu.databus);
    ct_assertequal(0xbbu, apu.cpu.a);
    ct_asserttrue(apu.cpu.signal.rw);
    ct_asserttrue(apu.cpu.signal.rdy);
    ct_assertequal(7, rc.count);
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

        ct_maketest(aligned_oam_sequence),
        ct_maketest(unaligned_oam_sequence),
    };

    return ct_makesuite(tests);
}
