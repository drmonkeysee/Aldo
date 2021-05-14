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

struct memorybank {
    struct memorybank *next;    // Next bank in the list
    size_t size;                // Bank Size
    uint8_t *mem;               // Bank Memory
    rpolicy *read;              // Memory-read policy; invoked if READ mode
    wpolicy *write;             // Memory-write policy; invoked if WRITE mode
    void *pctx;                 // Context object for RW policies
    enum memorymode mode;       // Memory access mode
    uint16_t addrmin,           // Min Address covered by this bank
             addrmax;           // Max Address covered by this bank
};

struct memorymap {
    size_t addrwidth,               // Address Width in bits
           pagewidth,               // Page Width in bits
           size,                    // Active Bank Count
           capacity,                // Total Bank Count (active and free)
           pagecount;               // Memory Page Count
    struct memorybank *banks,       // Active Banks
                      *freelist;    // Free Banks (available for reuse)
    uint16_t addrmask,              // Active Address Bits
             pagehash;              // Address-to-Page-Index Hash Factor
    struct memorybank *pages[];     // Array of Memory Pages
};

//
// Public Interface
//

memmap *memmap_new(void)
{
    struct memorymap *const self = malloc(sizeof *self);
    // NOTE: assume an initial capacity of 2 banks (RAM and ROM)
    *self = (struct memorymap){
        .capacity = 2,
        .banks = calloc(2, sizeof *self->banks),
    };
    return self;
}

void memmap_free(memmap *self)
{
    free(self->banks);
    free(self);
}

size_t memmap_size(memmap *self)
{
    assert(self != NULL);
    return self->size;
}

size_t memmap_capacity(memmap *self)
{
    assert(self != NULL);
    return self->capacity;
}

bool memmap_add(memmap *self, const membank *b)
{
    assert(self != NULL);
    assert(b != NULL);

    // TODO: determine if new banks will replace existing banks due to overlap
    // and figure out index of new bank

    // no overlap check capacity
    const size_t newsize = self->size + 1;
    if (newsize > self->capacity) {
        // K = 1.5 growth factor (i.e. N + N/2)
        self->capacity += self->capacity >> 1;
        self->banks = realloc(self->banks, self->capacity);
    }

    // finally add the new bank and shuffle everything around

    return false;
}

bool memmap_read(memmap *self, uint16_t addr, uint8_t *restrict d)
{
    return false;
}

bool memmap_write(memmap *self, uint16_t addr, uint8_t d)
{
    return false;
}

bool memmap_copy(memmap *self, size_t size, uint8_t buf[restrict size],
                 uint16_t minaddr, uint16_t maxaddr)
{
    return false;
}
