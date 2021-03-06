//
//  cpuhelp.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "cpuhelp.h"

#include "ciny.h"
#include "emu/bus.h"
#include "emu/bytes.h"
#include "emu/snapshot.h"

#include <stdbool.h>

static bool TestRead(const void *restrict ctx, uint16_t addr,
                     uint8_t *restrict d)
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

static bool TestWrite(void *ctx, uint16_t addr, uint8_t d)
{
    if (addr < MEMBLOCK_8KB) {
        ((uint8_t *)ctx)[addr & ADDRMASK_2KB] = d;
        return true;
    }
    return false;
}

static bus *restrict TestBus;
static struct busdevice Ram = {.read = TestRead, .write = TestWrite};
static struct busdevice Rom = {.read = TestRead};

// NOTE: one page of rom + extra to test page boundary addressing
uint8_t bigrom[] = {
    0xca,
    [255] = 0xfe, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
};

void setup_testbus(void)
{
    TestBus = bus_new(16, 3, MEMBLOCK_8KB, MEMBLOCK_32KB);
}

void teardown_testbus(void)
{
    bus_free(TestBus);
}

void setup_cpu(struct mos6502 *cpu, uint8_t *restrict ram,
               uint8_t *restrict rom)
{
    cpu_powerup(cpu);
    cpu->p.c = cpu->p.z = cpu->p.d = cpu->p.v = cpu->p.n = cpu->bflt = false;
    cpu->p.i = cpu->signal.rdy = cpu->presync = true;
    cpu->res = NIS_CLEAR;
    cpu->bus = TestBus;
    if (ram) {
        Ram.ctx = ram;
        bus_set(TestBus, 0x0, Ram);
    }
    if (rom) {
        Rom.ctx = rom;
        bus_set(TestBus, MEMBLOCK_32KB, Rom);
    }
}

int clock_cpu(struct mos6502 *cpu)
{
    int cycles = 0;
    do {
        cycles += cpu_cycle(cpu);
        // NOTE: catch instructions that run longer than possible
        ct_asserttrue(cycles <= MaxCycleCount);
    } while (!cpu->presync);
    return cycles;
}
