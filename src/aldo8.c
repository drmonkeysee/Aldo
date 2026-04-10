//
//  aldo8.c
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#include "aldo8.h"

#include "bytes.h"
#include "cart.h"
#include "cpu.h"
#include "ctrlsignal.h"

#include <stdint.h>

struct aldo_snapshot;

// The Aldo-8 fictional 8-bit console, useful for testing and tinkering with
// emulating a generic 6502-based system.
struct aldo_aldo8_console {
    aldo_cart *cart;            // Program Cartridge; Non-owning Pointer
    aldo_debugger *dbg;         // Debugger Context; Non-owning Pointer
    struct aldo_snapshot *snp;  // Console Snapshot; Non-owning Pointer
    FILE *tracelog;             // Optional trace log; Non-owning Pointer
    struct aldo_mos6502 cpu;    // MOS 6502 CPU
    enum aldo_execmode mode;    // Aldo8 execution mode
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
    uint8_t ram[ALDO_MEMBLOCK_32KB];    // Internal RAM
};
