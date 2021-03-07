//
//  cpuhelp.h
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#ifndef AldoTests_emu_cpuhelp_h
#define AldoTests_emu_cpuhelp_h

#include "emu/cpu.h"

#include <stdint.h>

extern const uint8_t bigrom[];

void setup_cpu(struct mos6502 *cpu);
int clock_cpu(struct mos6502 *cpu);

#endif
