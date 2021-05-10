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

// A Memory Map is a vector of Memory Banks

struct memorymap {
    size_t size, capacity;
    struct memorybank *banks;
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

bool memmap_addnbanks(memmap *self, size_t count,
                      const struct memorybank banks[count])
{
    assert(self != NULL);
    assert(count > 0);
    assert(banks != NULL);

    // TODO: determine if new banks will replace existing banks
    // any overlap must remove the existing bank, even if that creates holes

    const size_t newsize = self->size + count;
    if (newsize > self->capacity) {
        while (newsize >= self->capacity) {
            // K = 1.5 growth factor
            self->capacity *= 1.5;
        }
        self->banks = realloc(self->banks, self->capacity);
    }

    return false;
}
