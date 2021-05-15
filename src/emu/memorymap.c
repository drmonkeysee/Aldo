//
//  memorymap.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/9/21.
//

#include "memorymap.h"

#include <assert.h>
#include <stdlib.h>

struct partition {
    struct memlink link;    // Link to bus component
    uint16_t start;         // Partition start address
};

struct memorymap {
    size_t count;                   // Partition count
    uint16_t maxaddr;               // Max address covered by this map
    struct partition partitions[];  // Address space partitions
};

//
// Public Interface
//

const char *memmap_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case s: return e;
        MEMMAP_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

memmap *memmap_new(size_t addrwidth, size_t n, ...)
{
    assert(0 < addrwidth && addrwidth <= 16);
    assert(0 < n);

    return NULL;
}

void memmap_free(memmap *self)
{
    free(self);
}

size_t memmap_count(memmap *self)
{
    assert(self != NULL);

    return 0;
}

uint16_t memmap_maxaddr(memmap *self)
{
    assert(self != NULL);

    return 0;
}

int memmap_pstart(memmap *self, size_t i)
{
    assert(self != NULL);

    return 0;
}

extern inline bool memmap_set(memmap *, uint16_t, struct memlink);
bool memmap_swap(memmap *self, uint16_t addr, struct memlink ml,
                 struct memlink *prev)
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
