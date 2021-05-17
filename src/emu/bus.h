//
//  bus.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/15/21.
//

#ifndef Aldo_emu_bus_h
#define Aldo_emu_bus_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// X(symbol, value, error string)
#define EMUBUS_ERRCODE_X \
X(EMUBUS_PARTITION_RANGE, -1, "PARTITION OUT OF RANGE")

enum {
#define X(s, v, e) s = v,
    EMUBUS_ERRCODE_X
#undef X
};

// A Bus defines how address lines are wired to system components in a
// processor's address space; the bus is divided into a fixed-set of
// address-range partitions each linked to some device such as memory,
// switchable RAM/ROM banks, memory-mapped registers, I/O controllers, etc;

// although the partitions are fixed, devices can be swapped in and out
// of these partitions provided the device maps to the partition's address
// range; it is also possible for a partition to have no linked
// device, in which case the address range is not mapped to anything.
typedef struct addressbus bus;
typedef bool rpolicy(void *restrict, uint16_t, uint8_t *restrict);
typedef bool wpolicy(void *, uint16_t, uint8_t);

struct busdevice {
    rpolicy *read;  // Read policy (if NULL, device is write-only)
    wpolicy *write; // Write policy (if NULL, device is read-only)
    void *ctx;      // Policy context (non-owning pointer)
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *bus_errstr(int err);

// NOTE: n is partition count, while variadic arguments specify where the
// partition divisions are; thus variadic argument count is n - 1;
// e.g. (16, 4, 0x2000, 0x4000, 0x8000) ->
// - 16-bit address space
// - 4 partitions
// - mapped as [$0000 - $1FFF, $2000 - $3FFF, $4000 - $7FFF, $8000 - $FFFF]
bus *bus_new(int bitwidth, size_t n, ...);
void bus_free(bus *self);

size_t bus_count(bus *self);
uint16_t bus_maxaddr(bus *self);
int bus_pstart(bus *self, size_t i);

// NOTE: addr can be anywhere in the range of the target device's partition
bool bus_swap(bus *self, uint16_t addr, struct busdevice bd,
              struct busdevice *prev);
inline bool bus_set(bus *self, uint16_t addr, struct busdevice bd)
{
    return bus_swap(self, addr, bd, NULL);
}

bool bus_read(bus *self, uint16_t addr, uint8_t *restrict d);
bool bus_write(bus *self, uint16_t addr, uint8_t d);

bool bus_copy(bus *self, size_t size, uint8_t buf[restrict size],
              uint16_t minaddr, uint16_t maxaddr);

#endif
