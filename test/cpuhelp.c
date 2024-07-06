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
    if (addr < MEMBLOCK_8KB) {
        *d = ((const uint8_t *)ctx)[addr & ADDRMASK_2KB];
        return true;
    }
    if (MEMBLOCK_32KB <= addr) {
        *d = ((const uint8_t *)ctx)[addr & ADDRMASK_32KB];
        return true;
    }
    return false;
}

static bool test_write(void *ctx, uint16_t addr, uint8_t d)
{
    if (addr < MEMBLOCK_8KB) {
        ((uint8_t *)ctx)[addr & ADDRMASK_2KB] = d;
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

static bus *restrict TestBus;
static struct busdevice Ram = {.read = test_read, .write = test_write},
                        Rom = {.read = test_read};

//
// Public Interface
//

// NOTE: one page of rom + extra to test page boundary addressing
uint8_t BigRom[] = {
    0xca,
    [255] = 0xfe, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
};
int RomWriteCapture = -1;

void setup_testbus(void)
{
    TestBus = bus_new(BITWIDTH_64KB, 3, MEMBLOCK_8KB, MEMBLOCK_32KB);
}

void teardown_testbus(void)
{
    bus_free(TestBus);
}

void setup_cpu(struct mos6502 *cpu, uint8_t *restrict ram,
               uint8_t *restrict rom)
{
    cpu->mbus = TestBus;
    Ram.ctx = ram;
    bus_set(TestBus, 0x0, ram ? Ram : (struct busdevice){0});
    Rom.ctx = rom;
    bus_set(TestBus, MEMBLOCK_32KB, rom ? Rom : (struct busdevice){0});
    RomWriteCapture = -1;
    cpu_powerup(cpu);
    cpu->p.c = cpu->p.z = cpu->p.d = cpu->p.v = cpu->p.n = cpu->bcd =
        cpu->bflt = cpu->detached = false;
    cpu->p.i = cpu->signal.rdy = cpu->presync = true;
    cpu->rst = CSGS_CLEAR;
}

void enable_rom_wcapture(void)
{
    struct busdevice dv = Rom;
    dv.write = capture_rom_write;
    bus_set(TestBus, MEMBLOCK_32KB, dv);
}

int clock_cpu(struct mos6502 *cpu)
{
    int cycles = 0;
    do {
        cycles += cpu_cycle(cpu);
        // NOTE: catch instructions that run longer than possible
        ct_asserttrue(cycles <= MaxTCycle);
    } while (!cpu->presync);
    return cycles;
}
