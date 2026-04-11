//
//  aldo8.c
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#include "aldo8.h"

#include "bus.h"
#include "bytes.h"
#include "consoledef.h"
#include "cpu.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

// The Aldo-8 fictional 8-bit console, useful for testing and tinkering with
// emulating a generic 6502-based system.
struct aldo_aldo8_console {
    struct aldo_console_base extends;

    struct aldo_mos6502 cpu;            // MOS 6502 CPU
    uint8_t ram[ALDO_MEMBLOCK_32KB];    // Internal RAM
};

static bool ram_read(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    // addr=[$0000-$7FFF]
    assert(addr < ALDO_MEMBLOCK_32KB);

    *d = ((const uint8_t *)ctx)[addr];
    return true;
}

static bool ram_write(void *ctx, uint16_t addr, uint8_t d)
{
    // addr=[$0000-$7FFF]
    assert(addr < ALDO_MEMBLOCK_32KB);

    ((uint8_t *)ctx)[addr] = d;
    return true;
}

static size_t ram_copy(const void *restrict ctx, uint16_t addr, size_t count,
                       uint8_t dest[restrict count])
{
    // addr=[$0000-$7FFF]
    assert(addr < ALDO_MEMBLOCK_32KB);

    return aldo_bytecopy_bank(ctx, ALDO_BITWIDTH_32KB, addr, count, dest);
}

// TODO: these setup functions may work better as pointers?
static bool create_mbus(struct aldo_aldo8_console *self)
{
    /*
     * Memory Map
     * 16-bit Address Space = 64KB
     *   $0000 - $7FFF: 32KB RAM
     *   $8000 - $FFFF: 32KB Cart
     */
    self->cpu.mbus = aldo_bus_new(ALDO_BITWIDTH_64KB, 2, ALDO_MEMBLOCK_32KB);
    if (!self->cpu.mbus) return false;

    auto r = aldo_bus_set(self->cpu.mbus, 0, (struct aldo_busdevice){
        ram_read,
        ram_write,
        ram_copy,
        self->ram,
    });
    (void)r, assert(r);
    return true;
}

static void connect_cart(struct aldo_aldo8_console *self, aldo_cart *c)
{
    self->extends.cart = c;
    auto r = aldo_cart_mbus_connect(self->extends.cart, self->cpu.mbus);
    (void)r, assert(r);
    aldo_debug_sync_bus(self->extends.dbg);
}

static void disconnect_cart(struct aldo_aldo8_console *self)
{
    // Debugger may have been attached to a cart-less CPU bus so reset
    // debugger even if there is no existing cart.
    aldo_debug_reset(self->extends.dbg);
    if (!self->extends.cart) return;
    aldo_cart_mbus_disconnect(self->extends.cart, self->cpu.mbus);
    self->extends.cart = nullptr;
}

static bool setup(struct aldo_aldo8_console *self)
{
    if (!create_mbus(self)) return false;
    aldo_debug_cpu_connect(self->extends.dbg, &self->cpu);
    return true;
}

static void teardown(struct aldo_aldo8_console *self)
{
    disconnect_cart(self);
    aldo_debug_cpu_disconnect(self->extends.dbg);
    aldo_bus_free(self->cpu.mbus);
}

//
// MARK: - Public Interface
//

aldo_aldo8 *aldo_aldo8_new(aldo_debugger *dbg, FILE *tracelog)
{
    struct aldo_aldo8_console *self = malloc(sizeof *self);
    if (!self) return self;

    aldo_base_init(&self->extends, dbg, tracelog);

    self->cpu.bcd = true;
    if (!setup(self)) {
        aldo_aldo8_free(self);
        return nullptr;
    }
    return self;
}

void aldo_aldo8_free(aldo_aldo8 *self)
{
    assert(self != nullptr);

    teardown(self);
    free(self);
}
