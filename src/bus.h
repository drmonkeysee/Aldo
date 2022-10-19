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

typedef struct addressbus bus;

#include "bridgeopen.h"
struct busdevice {
    bool (*read)(const void *aldo_noalias, uint16_t, uint8_t *aldo_noalias);
    bool (*write)(void *, uint16_t, uint8_t);
    size_t (*dma)(const void *aldo_noalias, uint16_t, size_t, uint8_t[aldo_noalias]);
    void *ctx;  // Non-owning Pointer
};

// NOTE: n is partition count, while variadic arguments specify where the
// partition divisions are; thus variadic argument count is n - 1;
// e.g. (16, 4, 0x2000, 0x4000, 0x8000) ->
// - 16-bit address space
// - 4 partitions
// - mapped as [$0000 - $1FFF, $2000 - $3FFF, $4000 - $7FFF, $8000 - $FFFF]
bus *bus_new(int bitwidth, size_t n, ...) aldo_nothrow;
void bus_free(bus *self) aldo_nothrow;

// NOTE: addr can be anywhere in the range of the target device's partition
bool bus_swap(bus *self, uint16_t addr, struct busdevice bd,
              struct busdevice *prev) aldo_nothrow;
inline bool bus_set(bus *self, uint16_t addr, struct busdevice bd) aldo_nothrow
{
    return bus_swap(self, addr, bd, NULL);
}

bool bus_read(bus *self, uint16_t addr, uint8_t *aldo_noalias d) aldo_nothrow;
bool bus_write(bus *self, uint16_t addr, uint8_t d) aldo_nothrow;
size_t bus_dma(bus *self, uint16_t addr, size_t count,
               uint8_t dest[aldo_noalias_sz(count)]) aldo_nothrow;
#include "bridgeclose.h"

#endif
