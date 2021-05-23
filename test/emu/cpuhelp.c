//
//  cpuhelp.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "cpuhelp.h"

#include "ciny.h"
#include "emu/bus.h"
#include "emu/snapshot.h"
#include "emu/traits.h"

static bool TestRead(const void *restrict ctx, uint16_t addr,
                     uint8_t *restrict d)
{
    if (addr <= CpuRamMaxAddr) {
        *d = ((const uint8_t *)ctx)[addr & CpuRamAddrMask];
        return true;
    }
    if (CpuRomMinAddr <= addr && addr <= CpuRomMaxAddr) {
        *d = ((const uint8_t *)ctx)[addr & CpuRomAddrMask];
        return true;
    }
    return false;
}

static bool TestWrite(void *ctx, uint16_t addr, uint8_t d)
{
    if (addr <= CpuRamMaxAddr) {
        ((uint8_t *)ctx)[addr & CpuRamAddrMask] = d;
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
    TestBus = bus_new(16, 3, CpuRamMaxAddr + 1, CpuRomMinAddr);
}

void teardown_testbus(void)
{
    bus_free(TestBus);
}

void setup_cpu(struct mos6502 *cpu, uint8_t *restrict ram,
               uint8_t *restrict rom)
{
    cpu_powerup(cpu);
    cpu->a = cpu->x = cpu->y = cpu->s = cpu->pc = 0;
    cpu->p.c = cpu->p.z = cpu->p.d = cpu->p.v = cpu->p.n = cpu->bflt = false;
    cpu->p.i = cpu->signal.rdy = cpu->presync = true;
    cpu->irq = cpu->nmi = cpu->res = NIS_CLEAR;
    cpu->bus = TestBus;
    if (ram) {
        Ram.ctx = ram;
        bus_set(TestBus, 0x0, Ram);
    }
    if (rom) {
        Rom.ctx = rom;
        bus_set(TestBus, CpuRomMinAddr, Rom);
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
