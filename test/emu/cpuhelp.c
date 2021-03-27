//
//  cpuhelp.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#include "cpuhelp.h"

#include "ciny.h"
#include "emu/traits.h"

// NOTE: one page of rom + extra to test page boundary addressing
const uint8_t bigrom[] = {
    0xca,
    [255] = 0xfe, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
};

void setup_cpu(struct mos6502 *cpu)
{
    cpu_powerup(cpu);
    // NOTE: set the cpu ready to read instruction at 0x0
    cpu->pc = 0;
    cpu->signal.rdy = true;
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
