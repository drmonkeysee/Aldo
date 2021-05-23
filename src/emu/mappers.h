//
//  mappers.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#ifndef Aldo_emu_mappers_h
#define Aldo_emu_mappers_h

#include "bus.h"
#include "cart.h"

#include <stdint.h>
#include <stdio.h>

struct mapper {
    void (*dtor)(struct mapper *);
    struct busdevice (*make_cpudevice)(const struct mapper *);
    // TODO: temp helper function
    uint8_t *(*getprg)(const struct mapper *);
};

// NOTE: if create functions return non-zero error code, *m is unmodified
int rom_mapper_create(struct mapper **m, FILE *f);
int ines_mapper_create(struct mapper **m, const struct ines_header *header,
                       FILE *f);

#endif
