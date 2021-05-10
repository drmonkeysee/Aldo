//
//  memorymap.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/9/21.
//

#include "memorymap.h"

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
    return NULL;
}

void memmap_free(memmap *self)
{
    free(self);
}
