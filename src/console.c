//
//  console.c
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#include "consoledef.h"

#include "aldo8.h"

#include <assert.h>

//
// MARK: - Internal Interface
//

void aldo_base_init(struct aldo_console_base *self, aldo_debugger *dbg,
                    FILE *tracelog)
{
    assert(self != nullptr);
    assert(dbg != nullptr);

    self->cart = nullptr;
    self->dbg = dbg;
    self->tracelog = tracelog;
    self->halted = self->probe.rdy = true;
    self->tracefailed = self->probe.irq = self->probe.nmi = self->probe.rst = false;
}

//
// MARK: - Public Interface
//

aldo_console *aldo_console_new(enum aldo_console_type type, aldo_debugger *dbg,
                               FILE *tracelog)
{
    assert(dbg != nullptr);

    switch (type) {
    case ALDO_CONSOLE_ALDO8:
        return (struct aldo_console_base *)aldo_aldo8_new(dbg, tracelog);
    case ALDO_CONSOLE_NES:
        break;
    default:
        assert(((void)"INVALID CONSOLE TYPE", false));
        break;
    }
    return nullptr;
}
