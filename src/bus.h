//
//  bus.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/15/21.
//

#ifndef Aldo_bus_h
#define Aldo_bus_h

#include "bustype.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct aldo_busdevice {
    bool (*read)(void *restrict, uint16_t, uint8_t *restrict);
    bool (*write)(void *, uint16_t, uint8_t);
    size_t (*copy)(const void *restrict, uint16_t, size_t, uint8_t[restrict]);
    void *ctx;  // Non-owning Pointer
};

// NOTE: n is partition count, while variadic arguments specify where the
// partition divisions are; thus variadic argument count is n - 1;
// e.g. (16, 4, 0x2000, 0x4000, 0x8000) ->
// - 16-bit address space
// - 4 partitions
// - mapped as [$0000 - $1FFF, $2000 - $3FFF, $4000 - $7FFF, $8000 - $FFFF]
aldo_bus *aldo_bus_new(int bitwidth, size_t n, ...);
void aldo_bus_free(aldo_bus *self);

// NOTE: addr can be anywhere in the range of the target device's partition
bool aldo_bus_swap(aldo_bus *self, uint16_t addr, struct aldo_busdevice bd,
                   struct aldo_busdevice *prev);
inline bool aldo_bus_set(aldo_bus *self, uint16_t addr,
                         struct aldo_busdevice bd)
{
    return aldo_bus_swap(self, addr, bd, NULL);
}
inline bool aldo_bus_clear(aldo_bus *self, uint16_t addr)
{
    return aldo_bus_set(self, addr, (struct aldo_busdevice){0});
}

bool aldo_bus_read(aldo_bus *self, uint16_t addr, uint8_t *restrict d);
bool aldo_bus_write(aldo_bus *self, uint16_t addr, uint8_t d);
size_t aldo_bus_copy(aldo_bus *self, uint16_t addr, size_t count,
                     uint8_t dest[restrict count]);

#endif
