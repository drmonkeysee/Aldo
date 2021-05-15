//
//  memorymap.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/9/21.
//

#include "memorymap.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct partition {
    struct busdevice device;    // Device connected to this bus partition
    uint16_t addrmin,           // Min partition address
             addrmax;           // Max partition address
};

struct memorymap {
    size_t size;                    // Partition count
    uint16_t maxaddr;               // Max address covered by this map
    struct partition partitions[];  // Address space partitions
};

//
// Public Interface
//

memmap *memmap_new(size_t addrwidth)
{
    assert(0 < addrwidth && addrwidth <= 16);

    return NULL;
}

void memmap_free(memmap *self)
{
    free(self);
}

bool memmap_add(memmap *self, struct busdevice d)
{
    assert(self != NULL);

    return false;
}

bool memmap_read(memmap *self, uint16_t addr, uint8_t *restrict d)
{
    assert(self != NULL);

    return false;
}

bool memmap_write(memmap *self, uint16_t addr, uint8_t d)
{
    assert(self != NULL);

    return false;
}

bool memmap_copy(memmap *self, size_t size, uint8_t buf[restrict size],
                 uint16_t minaddr, uint16_t maxaddr)
{
    assert(self != NULL);
    assert(buf != NULL);

    return false;
}
