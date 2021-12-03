//
//  cpuhelp.h
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#ifndef AldoTests_cpuhelp_h
#define AldoTests_cpuhelp_h

#include "cpu.h"

#include <stdint.h>

extern uint8_t BigRom[];

void setup_cpu(struct mos6502 *cpu, uint8_t *restrict ram,
               uint8_t *restrict rom);
int clock_cpu(struct mos6502 *cpu);

#endif
