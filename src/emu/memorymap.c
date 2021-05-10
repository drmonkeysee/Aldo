//
//  memorymap.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/9/21.
//

#include "memorymap.h"

#include <assert.h>
#include <stdlib.h>

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
    *self = (struct memorymap){0};
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

    return false;
}
