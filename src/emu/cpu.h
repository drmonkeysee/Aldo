//
//  cpu.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_emu_cpu_h
#define Aldo_emu_cpu_h

#include "snapshot.h"

#include <stdbool.h>
#include <stdint.h>

// NOTE: The MOS6502 processor is a little-endian
// 8-bit CPU with a 16-bit addressing space.

struct mos6502 {
    // NOTE: Microprocessor Internals
    // direct modeling of real CPU physical elements.
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
             u: 1,      // (5) Unused
             v: 1,      // (6) Overflow
             n: 1;      // (7) Sign
    } p;                // Status

    // Datapath
    // abstractions over the internal workings of
    // instruction fetching, execution, and signalling.
    uint16_t addrbus;   // Value put on the address pins on clock phase ϕ1
    uint8_t databus,    // Value put on the data pins on clock phase ϕ2
            opc,        // Current Opcode
            opl,        // Opcode operand low byte
            oph,        // Opcode operand high byte
            t;          // Current instruction cycle (T0, T1, T2...)
    struct {
        bool irq: 1,    // Maskable Interrupt Signal (input, inverted)
             nmi: 1,    // Nonmaskable Interrupt Signal (input, inverted)
             res: 1,    // Reset Signal (input, inverted)
             rw: 1,     // Read/Write Signal (output, read high)
             sync: 1;   // SYNC (instruction fetch) Signal (output)
    } signal;

    // Buses
    // external components connected to the CPU pins by the console.
    uint8_t *ram,       // RAM Bus
            *cart;      // TODO: temp pointer to fake cartridge
};

void cpu_powerup(struct mos6502 *self);

void cpu_reset(struct mos6502 *self);

void cpu_snapshot(const struct mos6502 *self, struct console_state *snapshot);

#endif
