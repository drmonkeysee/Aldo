//
//  consoledef.h
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#ifndef Aldo_consoledef_h
#define Aldo_consoledef_h

#include "console.h"

// Console Base Definition, providing a common interface and data type for
// emulation of all supported Aldo consoles.
struct aldo_console_base {
    enum aldo_console_type type;
    aldo_cart *cart;            // Program Cartridge; Non-owning Pointer
    aldo_debugger *dbg;         // Debugger Context; Non-owning Pointer
    struct aldo_snapshot *snp;  // Console Snapshot; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    enum aldo_execmode mode;    // Console execution mode
    struct {
        bool
            irq: 1,                     // IRQ Probe
            nmi: 1,                     // NMI Probe
            rdy: 1,                     // READY Probe
            rst: 1;                     // RESET Probe
    } probe;                            // Interrupt Input Probes (active high)
    bool
        halted,                         // Whether the emulator is suspended
        tracefailed;                    // Trace log I/O failed during run
};

// Internal Console Base Operations

void aldo_base_init(struct aldo_console_base *self, aldo_debugger *dbg,
                    FILE *tracelog);

#endif
