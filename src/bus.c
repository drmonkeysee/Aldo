//
//  bus.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/15/21.
//

#include "bus.h"

#include "bytes.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

struct hardwarebus {
    size_t count;
    uint16_t maxaddr;
    struct partition {
        struct busdevice device;
        uint16_t start;
    } partitions[];
};

static struct partition *find(struct hardwarebus *self, uint16_t addr)
{
    for (size_t i = self->count - 1; i > 0; --i) {
        if (addr >= self->partitions[i].start) return self->partitions + i;
    }
    // NOTE: return the first partition if we got this far
    return self->partitions;
}

//
// MARK: - Public Interface
//

bus *bus_new(int bitwidth, size_t n, ...)
{
    assert(0 < bitwidth && bitwidth <= ALDO_BITWIDTH_64KB);
    assert(0 < n);

    size_t psize = sizeof(struct partition) * n;
    struct hardwarebus *self = malloc(sizeof *self + psize);
    *self = (struct hardwarebus){
        .count = n,
        .maxaddr = (uint16_t)(1 << bitwidth) - 1,
    };
    self->partitions[0] = (struct partition){0};
    va_list args;
    va_start(args, n);
    for (size_t i = 1; i < n; ++i) {
        self->partitions[i] = (struct partition){
            .start = (uint16_t)va_arg(args, unsigned int),
        };
    }
    va_end(args);

    return self;
}

void bus_free(bus *self)
{
    free(self);
}

extern inline bool bus_set(bus *, uint16_t, struct busdevice);
extern inline bool bus_clear(bus *, uint16_t);
bool bus_swap(bus *self, uint16_t addr, struct busdevice bd,
              struct busdevice *prev)
{
    assert(self != NULL);

    if (addr > self->maxaddr) return false;

    struct partition *target = find(self, addr);
    if (prev) {
        *prev = target->device;
    }
    target->device = bd;
    return true;
}

bool bus_read(bus *self, uint16_t addr, uint8_t *restrict d)
{
    assert(self != NULL);
    assert(d != NULL);

    if (addr > self->maxaddr) return false;

    struct partition *target = find(self, addr);
    return target->device.read
            ? target->device.read(target->device.ctx, addr, d)
            : false;
}

bool bus_write(bus *self, uint16_t addr, uint8_t d)
{
    assert(self != NULL);

    if (addr > self->maxaddr) return false;

    struct partition *target = find(self, addr);
    return target->device.write
            ? target->device.write(target->device.ctx, addr, d)
            : false;
}

size_t bus_copy(bus *self, uint16_t addr, size_t count,
                uint8_t dest[restrict count])
{
    assert(self != NULL);
    assert(dest != NULL);
    assert(count < ALDO_MEMBLOCK_64KB);

    if (addr > self->maxaddr || count == 0) return 0;

    struct partition *target = find(self, addr);
    return target->device.copy
            ? target->device.copy(target->device.ctx, addr, count, dest)
            : 0;
}
