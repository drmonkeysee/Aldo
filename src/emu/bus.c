//
//  bus.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/15/21.
//

#include "bus.h"

#include <assert.h>
#include <stdlib.h>

struct partition {
    struct busdevice device;    // Bus device wired to this partition
    uint16_t start;             // Partition start address
};

struct addressbus {
    size_t count;                   // Partition count
    uint16_t maxaddr;               // Max address decodable on this bus
    struct partition partitions[];  // Address space partitions
};

//
// Public Interface
//

const char *bus_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case s: return e;
        BUS_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

bus *bus_new(int bitwidth, size_t n, ...)
{
    assert(0 < bitwidth && bitwidth <= 16);
    assert(0 < n);

    return NULL;
}

void bus_free(bus *self)
{
    free(self);
}

size_t bus_count(bus *self)
{
    assert(self != NULL);

    return 0;
}

uint16_t bus_maxaddr(bus *self)
{
    assert(self != NULL);

    return 0;
}

int bus_pstart(bus *self, size_t i)
{
    assert(self != NULL);

    return 0;
}

extern inline bool bus_set(bus *, uint16_t, struct busdevice);
bool bus_swap(bus *self, uint16_t addr, struct busdevice bd,
              struct busdevice *prev)
{
    assert(self != NULL);

    return false;
}

bool bus_read(bus *self, uint16_t addr, uint8_t *restrict d)
{
    assert(self != NULL);

    return false;
}

bool bus_write(bus *self, uint16_t addr, uint8_t d)
{
    assert(self != NULL);

    return false;
}

bool bus_copy(bus *self, size_t size, uint8_t buf[restrict size],
              uint16_t minaddr, uint16_t maxaddr)
{
    assert(self != NULL);
    assert(buf != NULL);

    return false;
}
