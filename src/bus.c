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

struct aldo_hardwarebus {
    size_t count;
    uint16_t maxaddr;
    struct partition {
        struct aldo_busdevice device;
        uint16_t start;
    } partitions[];
};

static struct partition *find(struct aldo_hardwarebus *self, uint16_t addr)
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

aldo_bus *aldo_bus_new(int bitwidth, size_t n, ...)
{
    assert(0 < bitwidth && bitwidth <= ALDO_BITWIDTH_64KB);
    assert(0 < n);

    size_t psize = sizeof(struct partition) * n;
    struct aldo_hardwarebus *self = malloc(sizeof *self + psize);
    if (!self) return self;

    *self = (struct aldo_hardwarebus){
        .count = n,
        .maxaddr = (uint16_t)((1 << bitwidth) - 1),
    };
    self->partitions[0] = (struct partition){};
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

void aldo_bus_free(aldo_bus *self)
{
    free(self);
}

extern inline bool aldo_bus_set(aldo_bus *, uint16_t, struct aldo_busdevice);
extern inline bool aldo_bus_clear(aldo_bus *, uint16_t);
bool aldo_bus_swap(aldo_bus *self, uint16_t addr, struct aldo_busdevice bd,
                   struct aldo_busdevice *prev)
{
    assert(self != nullptr);

    if (addr > self->maxaddr) return false;

    auto target = find(self, addr);
    if (prev) {
        *prev = target->device;
    }
    target->device = bd;
    return true;
}

bool aldo_bus_read(aldo_bus *self, uint16_t addr, uint8_t *restrict d)
{
    assert(self != nullptr);
    assert(d != nullptr);

    if (addr > self->maxaddr) return false;

    auto target = find(self, addr);
    return target->device.read
            ? target->device.read(target->device.ctx, addr, d)
            : false;
}

bool aldo_bus_write(aldo_bus *self, uint16_t addr, uint8_t d)
{
    assert(self != nullptr);

    if (addr > self->maxaddr) return false;

    auto target = find(self, addr);
    return target->device.write
            ? target->device.write(target->device.ctx, addr, d)
            : false;
}

size_t aldo_bus_copy(aldo_bus *self, uint16_t addr, size_t count,
                     uint8_t dest[restrict count])
{
    assert(self != nullptr);
    assert(dest != nullptr);
    assert(count < ALDO_MEMBLOCK_64KB);

    if (addr > self->maxaddr || count == 0) return 0;

    auto target = find(self, addr);
    return target->device.copy
            ? target->device.copy(target->device.ctx, addr, count, dest)
            : 0;
}
