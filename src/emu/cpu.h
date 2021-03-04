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

// The MOS6502 processor is a little-endian
// 8-bit CPU with a 16-bit addressing space.

struct mos6502 {
    // CPU registers and flags
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

    // Datapath: abstract representation of instruction fetching,
    // execution, and signaling.
    uint16_t addrbus;   // Word put on the address pins on clock phase ϕ1
    uint8_t databus,    // Byte put on the data pins on clock phase ϕ2
            opc,        // Opcode
            adl,        // Address low latch (held when loading address high)
            alu;        // ALU result latch (held during internal ALU
                        // operations, such as indexed-addressing).
    int8_t t;           // Instruction sequence cycle (T0, T1, T2...)
    struct {
        bool irq: 1,    // Maskable Interrupt Signal (input, inverted)
             nmi: 1,    // Nonmaskable Interrupt Signal (input, inverted)
             res: 1,    // Reset Signal (input, inverted)
             rdy: 1,    // Ready Signal (input, low to halt cpu)
             rw: 1,     // Read/Write Signal (output, read high)
             sync: 1;   // SYNC (instruction fetch) Signal (output)
    } signal;

    // Internals: control flags and other helper fields that do
    // not correspond directly to CPU components.
    bool dflt,          // Data fault; read/write to unmapped address
         presync;       // Pre-sync cycle; primes the CPU to treat
                        // the following cycle as an opcode fetch (T0).

    // Buses: external components connected to the CPU pins
    uint8_t *ram;           // RAM Bus
    const uint8_t *cart;    // TODO: temp pointer to fake cartridge
};

void cpu_powerup(struct mos6502 *self);
void cpu_reset(struct mos6502 *self);

int cpu_cycle(struct mos6502 *self);

void cpu_snapshot(const struct mos6502 *self, struct console_state *snapshot);

#endif
