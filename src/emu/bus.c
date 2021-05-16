//
//  bus.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/15/21.
//

#include "bus.h"

#include <assert.h>
#include <stdarg.h>
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
        EMUBUS_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

bus *bus_new(int bitwidth, size_t n, ...)
{
    assert(0 < bitwidth && bitwidth <= 16);
    assert(0 < n);

    const size_t psize = sizeof(struct partition) * n;
    struct addressbus *const self = malloc(sizeof *self + psize);
    *self = (struct addressbus){
        .count = n,
        .maxaddr = (1 << bitwidth) - 1,
    };
    self->partitions[0] = (struct partition){0};
    va_list args;
    va_start(args, n);
    for (size_t i = 1; i < n; ++i) {
        self->partitions[i] = (struct partition){
            .start = va_arg(args, unsigned int),
        };
    }
    va_end(args);

    return self;
}

void bus_free(bus *self)
{
    free(self);
}

size_t bus_count(bus *self)
{
    assert(self != NULL);

    return self->count;
}

uint16_t bus_maxaddr(bus *self)
{
    assert(self != NULL);

    return self->maxaddr;
}

int bus_pstart(bus *self, size_t i)
{
    assert(self != NULL);

    return i < self->count
           ? self->partitions[i].start
           : EMUBUS_PARTITION_RANGE;
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
