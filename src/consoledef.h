//
//  consoledef.h
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#ifndef Aldo_consoledef_h
#define Aldo_consoledef_h

#include "console.h"
#include "cpu.h"

// Console Base Definition, providing a common interface and data type for
// emulation of all supported Aldo consoles.
struct aldo_console_base {
    // Console Type Header
    enum aldo_console_type type;
    // Optional callbaks for type-specific functionality
    void (*conn)(aldo_console *);   // Cart Connect
    void (*dconn)(aldo_console *);  // Cart Disconnect
    void (*dtor)(aldo_console *);   // Console Cleanup

    // Console System Parameters
    struct {
        size_t ramsize;
        int cyclefactor, framefactor, screenh, screenw;
    } params;

    // Console Components
    aldo_cart *cart;            // Program Cartridge; Non-owning Pointer
    aldo_debugger *dbg;         // Debugger Context; Non-owning Pointer
    struct aldo_snapshot *snp;  // Console Snapshot; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    struct aldo_mos6502 cpu;    // MOS 6502 CPU Core
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

#endif
