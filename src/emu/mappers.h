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
    uint16_t (*prgbank)(const struct mapper *, size_t,
                        const uint8_t *restrict *);
    bool (*cpu_connect)(struct mapper *, bus *, uint16_t);
    void (*cpu_disconnect)(const struct mapper *, bus *, uint16_t);
};

// NOTE: if create functions return non-zero error code, *m is unmodified
int mapper_raw_create(struct mapper **m, FILE *f);
int mapper_ines_create(struct mapper **m, const struct ines_header *header,
                       FILE *f);

#endif
