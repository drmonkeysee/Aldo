//
//  cpuhelp.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "cpuhelp.h"

#include "apu.h"
#include "bus.h"
#include "bytes.h"
#include "ciny.h"
#include "cpu.h"
#include "ctrlsignal.h"

static bool test_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    if (addr < ALDO_MEMBLOCK_8KB) {
        *d = ((const uint8_t *)ctx)[addr & ALDO_ADDRMASK_2KB];
        return true;
    }
    if (ALDO_MEMBLOCK_32KB <= addr) {
        *d = ((const uint8_t *)ctx)[addr & ALDO_ADDRMASK_32KB];
        return true;
    }
    return false;
}

static bool test_write(void *ctx, uint16_t addr, uint8_t d)
{
    if (addr < ALDO_MEMBLOCK_8KB) {
        ((uint8_t *)ctx)[addr & ALDO_ADDRMASK_2KB] = d;
        return true;
    }
    return false;
}

static bool capture_rom_write(void *, uint16_t, uint8_t d)
{
    RomWriteCapture = d;
    return false;
}

static aldo_bus *restrict TestBus;
static struct aldo_busdevice
    Ram = {.read = test_read, .write = test_write}, Rom = {.read = test_read};

static void connect_cpu(struct aldo_mos6502 *cpu, uint8_t *restrict ram,
                        uint8_t *restrict rom)
{
    cpu->mbus = TestBus;
    Ram.ctx = ram;
    aldo_bus_set(TestBus, 0x0, ram ? Ram : (struct aldo_busdevice){});
    Rom.ctx = rom;
    aldo_bus_set(TestBus, ALDO_MEMBLOCK_32KB,
                 rom ? Rom : (struct aldo_busdevice){});
    RomWriteCapture = -1;
}

static void reset_cpu(struct aldo_mos6502 *cpu)
{
    cpu->p.c = cpu->p.z = cpu->p.d = cpu->p.v = cpu->p.n = cpu->bcd = false;
    cpu->p.i = cpu->presync = true;
    cpu->rst = ALDO_SIG_CLEAR;
}

//
// MARK: - Public Interface
//

// one page of rom + extra to test page boundary addressing
uint8_t BigRom[] = {
    0xca,
    [255] = 0xfe, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
};
int RomWriteCapture = -1;

void setup_testbus()
{
    // basic RAM space, APU registers, and ROM space
    TestBus = aldo_bus_new(ALDO_BITWIDTH_64KB, 4, ALDO_MEMBLOCK_8KB,
                           ALDO_MEMBLOCK_16KB, ALDO_MEMBLOCK_32KB);
}

void teardown_testbus()
{
    aldo_bus_free(TestBus);
}

void setup_cpu(struct aldo_mos6502 *cpu, uint8_t *restrict ram,
               uint8_t *restrict rom)
{
    connect_cpu(cpu, ram, rom);
    aldo_cpu_powerup(cpu);
    reset_cpu(cpu);
}

void enable_rom_wcapture()
{
    auto dv = Rom;
    dv.write = capture_rom_write;
    aldo_bus_set(TestBus, ALDO_MEMBLOCK_32KB, dv);
}

int exec_cpu(struct aldo_mos6502 *cpu)
{
    auto cycles = 0;
    do {
        cycles += aldo_cpu_cycle(cpu);
        // catch instructions that run longer than possible
        ct_asserttrue(cycles <= Aldo_MaxTCycle);
    } while (!cpu->presync);
    return cycles;
}

void setup_apu(struct aldo_rp2a03 *apu, uint8_t *restrict ram,
               uint8_t *restrict rom)
{
    connect_cpu(&apu->cpu, ram, rom);
    aldo_apu_connect(apu);
    aldo_apu_powerup(apu);
    reset_cpu(&apu->cpu);
}
