//
//  cpu.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_emu_cpu_h
#define Aldo_emu_cpu_h

#include <stdbool.h>
#include <stdint.h>

struct mos6502 {
    uint16_t pc;        // Program Counter
    uint8_t a,          // Accumulator
            s,          // Stack Pointer
            x,          // X Index
            y;          // Y Index
    struct {
        bool c: 1,      // (0) Carry
             z: 1,      // (1) Zero
             i: 1,      // (2) Interrupt Disable
             d: 1,      // (3) Decimal (disabled on the NES)
             b: 1,      // (4) Break
              : 1,      // (5) Unused
             v: 1,      // (6) Overflow
             n: 1;      // (7) Sign
    } p;                // Status Register
    uint8_t ram[0x800]; // 2 KB RAM
};

#endif
