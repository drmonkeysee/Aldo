//
//  cpuhelp.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "cpuhelp.h"

#include "bus.h"
#include "bytes.h"
#include "ciny.h"
#include "ctrlsignal.h"

#include <stdbool.h>

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

static bool capture_rom_write(void *ctx, uint16_t addr, uint8_t d)
{
    (void)ctx, (void)addr;
    RomWriteCapture = d;
    return false;
}

static aldo_bus *restrict TestBus;
static struct aldo_busdevice
    Ram = {.read = test_read, .write = test_write}, Rom = {.read = test_read};

//
// MARK: - Public Interface
//

// NOTE: one page of rom + extra to test page boundary addressing
uint8_t BigRom[] = {
    0xca,
    [255] = 0xfe, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
};
int RomWriteCapture = -1;

void setup_testbus(void)
{
    TestBus = aldo_bus_new(ALDO_BITWIDTH_64KB, 3, ALDO_MEMBLOCK_8KB,
                           ALDO_MEMBLOCK_32KB);
}

void teardown_testbus(void)
{
    aldo_bus_free(TestBus);
}

void setup_cpu(struct aldo_mos6502 *cpu, uint8_t *restrict ram,
               uint8_t *restrict rom)
{
    cpu->mbus = TestBus;
    Ram.ctx = ram;
    aldo_bus_set(TestBus, 0x0, ram ? Ram : (struct aldo_busdevice){0});
    Rom.ctx = rom;
    aldo_bus_set(TestBus, ALDO_MEMBLOCK_32KB,
                 rom ? Rom : (struct aldo_busdevice){0});
    RomWriteCapture = -1;
    aldo_cpu_powerup(cpu);
    cpu->p.c = cpu->p.z = cpu->p.d = cpu->p.v = cpu->p.n = cpu->bcd =
        cpu->bflt = cpu->detached = false;
    cpu->p.i = cpu->signal.rdy = cpu->presync = true;
    cpu->rst = ALDO_SIG_CLEAR;
}

void enable_rom_wcapture(void)
{
    struct aldo_busdevice dv = Rom;
    dv.write = capture_rom_write;
    aldo_bus_set(TestBus, ALDO_MEMBLOCK_32KB, dv);
}

int exec_cpu(struct aldo_mos6502 *cpu)
{
    int cycles = 0;
    do {
        cycles += aldo_cpu_cycle(cpu);
        // NOTE: catch instructions that run longer than possible
        ct_asserttrue(cycles <= Aldo_MaxTCycle);
    } while (!cpu->presync);
    return cycles;
}
