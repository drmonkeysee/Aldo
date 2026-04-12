//
//  emu.h
//  Aldo
//
//  Created by Brandon Stansbury on 12/10/22.
//

#ifndef Aldo_cli_emu_h
#define Aldo_cli_emu_h

#include "debug.h"
#include "cart.h"
#include "cliargs.h"
#include "console.h"
#include "snapshot.h"

struct emulator {
    const struct cliargs *args; // Non-owning Pointer
    aldo_cart *cart;            // Non-owning Pointer
    aldo_debugger *debugger;
    aldo_console *console;
    struct aldo_snapshot snapshot;
};

#endif
