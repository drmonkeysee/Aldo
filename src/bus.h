//
//  bus.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/15/21.
//

#ifndef Aldo_bus_h
#define Aldo_bus_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct hardwarebus bus;

#include "bridgeopen.h"
struct busdevice {
    bool (*read)(void *br_noalias, uint16_t, uint8_t *br_noalias) br_nothrow;
    bool (*write)(void *, uint16_t, uint8_t) br_nothrow;
    size_t (*copy)(const void *br_noalias, uint16_t, size_t,
                   uint8_t[br_noalias]) br_nothrow;
    void *ctx;  // Non-owning Pointer
};

// NOTE: n is partition count, while variadic arguments specify where the
// partition divisions are; thus variadic argument count is n - 1;
// e.g. (16, 4, 0x2000, 0x4000, 0x8000) ->
// - 16-bit address space
// - 4 partitions
// - mapped as [$0000 - $1FFF, $2000 - $3FFF, $4000 - $7FFF, $8000 - $FFFF]
br_ownresult
bus *bus_new(int bitwidth, size_t n, ...) br_nothrow;
void bus_free(bus *self) br_nothrow;

// NOTE: addr can be anywhere in the range of the target device's partition
bool bus_swap(bus *self, uint16_t addr, struct busdevice bd,
              struct busdevice *prev) br_nothrow;
inline bool bus_set(bus *self, uint16_t addr, struct busdevice bd) br_nothrow
{
    return bus_swap(self, addr, bd, NULL);
}
inline bool bus_clear(bus *self, uint16_t addr) br_nothrow
{
    return bus_set(self, addr, br_empty(struct busdevice));
}

bool bus_read(bus *self, uint16_t addr, uint8_t *br_noalias d) br_nothrow;
bool bus_write(bus *self, uint16_t addr, uint8_t d) br_nothrow;
size_t bus_copy(bus *self, uint16_t addr, size_t count,
                uint8_t dest[br_noalias_sz(count)]) br_nothrow;
#include "bridgeclose.h"

#endif
