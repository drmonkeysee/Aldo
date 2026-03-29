//
//  cpuhelp.h
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 3/7/21.
//

#ifndef AldoTests_cpuhelp_h
#define AldoTests_cpuhelp_h

#include <stdint.h>

extern uint8_t BigRom[];
extern int RomWriteCapture;

struct aldo_mos6502;
struct aldo_rp2a03;

void setup_cpu(struct aldo_mos6502 *cpu, uint8_t *restrict ram,
               uint8_t *restrict rom);
void enable_rom_wcapture();
int exec_cpu(struct aldo_mos6502 *cpu);

void setup_apu(struct aldo_rp2a03 *apu, uint8_t *restrict ram,
               uint8_t *restrict rom);
int cycle_sync_apu(struct aldo_rp2a03 *apu);

#endif
