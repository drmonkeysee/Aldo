//
//  cpu.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_cpu_h
#define Aldo_cpu_h

#include "bus.h"
#include "ctrlsignal.h"
#include "decode.h"
#include "snapshot.h"

#include <stdbool.h>
#include <stdint.h>

// The MOS6502 processor is a little-endian
// 8-bit CPU with a 16-bit addressing space.
struct aldo_mos6502 {
    // Main Bus: external components connected to the CPU pins
    bus *mbus;          // Non-owning Pointer

    // CPU registers and flags
    uint16_t pc;        // Program Counter
    uint8_t a,          // Accumulator
            s,          // Stack Pointer
            x,          // X-Index
            y;          // Y-Index
    struct {
        bool
            c: 1,       // (0) Carry
            z: 1,       // (1) Zero
            i: 1,       // (2) Interrupt Disable
            d: 1;       // (3) Decimal (disabled on the NES)
        int: 2;         // (4,5) Break/Unused (cannot be set directly)
        bool
            v: 1,       // (6) Overflow
            n: 1;       // (7) Sign
    } p;                // Status

    // Datapath: abstract representation of instruction fetching,
    // execution, and signaling.
    int8_t t;           // Instruction sequence cycle (T0, T1, T2...)
    enum aldo_sigstate
        irq,            // IRQ detection latch
        nmi,            // NMI detection latch
        rst;            // RESET detection latch
    uint16_t
        addrbus,        // Word put on the address pins on clock phase ϕ1
        addrinst;       // Address of current instruction
    uint8_t databus,    // Byte put on the data pins on clock phase ϕ2
            opc,        // Opcode
            adl,        // Address latch low
            adh,        // Address latch high
            adc;        // Address carry
    struct {
        bool
            irq: 1,     // Maskable Interrupt Signal (input, inverted)
            nmi: 1,     // Nonmaskable Interrupt Signal (input, inverted)
            rst: 1,     // Reset Signal (input, inverted)
            rdy: 1,     // Ready Signal (input, low to halt cpu)
            rw: 1,      // Read/Write Signal (output, read high)
            sync: 1;    // Sync (instruction fetch) Signal (output)
    } signal;

    // Internals: control flags and other helper fields that do
    // not correspond directly to CPU components.
    bool
        bcd,            // BCD (binary-coded decimal) mode is supported
        bflt,           // Bus fault; read/write to unmapped or invalid address
        detached,       // Run CPU in detached mode, used mostly for peek-mode:
                        // - memory-writes disabled (includes the stack!)
                        // - bus side-effects suppressed
                        // - interrupts never serviced (except RST)
                        // - branches forced
        presync;        // Pre-sync cycle; primes the CPU to treat
                        // the following cycle as an opcode fetch (T0).
};

struct aldo_peekresult {
    enum aldo_addrmode mode;
    uint16_t interaddr, finaladdr;
    uint8_t data;
    bool busfault;
};

#include "bridgeopen.h"
br_libexport
extern const int Aldo_MaxTCycle;

void aldo_cpu_powerup(struct aldo_mos6502 *self) br_nothrow;

int aldo_cpu_cycle(struct aldo_mos6502 *self) br_nothrow;

bool aldo_cpu_jammed(const struct aldo_mos6502 *self) br_nothrow;
void aldo_cpu_snapshot(const struct aldo_mos6502 *self,
                       struct aldo_snapshot *snp) br_nothrow;

void aldo_cpu_peek_start(struct aldo_mos6502 *br_noalias self,
                         struct aldo_mos6502 *br_noalias restore) br_nothrow;
struct aldo_peekresult aldo_cpu_peek(struct aldo_mos6502 *self,
                                     uint16_t addr) br_nothrow;
void aldo_cpu_peek_end(struct aldo_mos6502 *br_noalias self,
                       struct aldo_mos6502 *br_noalias restore) br_nothrow;
#include "bridgeclose.h"

#endif
