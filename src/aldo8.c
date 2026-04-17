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
#include "snapshot.h"

#include <assert.h>
#include <stdint.h>

// The Aldo-8 fictional 8-bit console, useful for testing and tinkering with
// emulating a generic 6502-based system.
struct aldo_aldo8_console {
    struct aldo_console_base extends;

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

static bool create_mbus(struct aldo_aldo8_console *self)
{
    /*
     * Memory Map
     * 16-bit Address Space = 64KB
     *   $0000 - $7FFF: 32KB RAM
     *   $8000 - $FFFF: 32KB Cart
     */
    self->extends.cpu.mbus = aldo_bus_new(ALDO_BITWIDTH_64KB, 2, ALDO_MEMBLOCK_32KB);
    if (!self->extends.cpu.mbus) return false;

    auto r = aldo_bus_set(self->extends.cpu.mbus, 0, (struct aldo_busdevice){
        ram_read,
        ram_write,
        ram_copy,
        self->ram,
    });
    (void)r, assert(r);
    return true;
}

//
// MARK: - Public Interface
//

const size_t Aldo_Aldo8Size = sizeof(struct aldo_aldo8_console);

bool aldo_aldo8_init(aldo_aldo8 *self)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_ALDO8);

    self->extends.params.ramsize = aldo_arrsz(self->ram);
    return create_mbus(self);
}

void aldo_aldo8_powerup(aldo_aldo8 *self, bool zeroram)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_ALDO8);

    if (zeroram) {
        aldo_memclr(self->ram);
    }
}

void aldo_aldo8_snapshot_core(aldo_aldo8 *self)
{
    assert(self != nullptr);
    assert(self->extends.type == ALDO_CONSOLE_ALDO8);

    self->extends.snp->mem.ram = self->ram;
}
