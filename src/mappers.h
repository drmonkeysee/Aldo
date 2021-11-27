//
//  mappers.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#ifndef Aldo_mappers_h
#define Aldo_mappers_h

#include "bus.h"
#include "cart.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct mapper {
    void (*dtor)(struct mapper *);
    size_t (*prgbank)(const struct mapper *, size_t, const uint8_t **);
    bool (*cpu_connect)(struct mapper *, bus *, uint16_t);
    void (*cpu_disconnect)(const struct mapper *, bus *, uint16_t);

    // NOTE: optional protocol
    size_t (*chrbank)(const struct mapper *, size_t, const uint8_t **);
};

// NOTE: if create functions return non-zero error code, *m is unmodified
int mapper_raw_create(struct mapper **m, FILE *f);
int mapper_ines_create(struct mapper **m, struct ines_header *header, FILE *f);

#endif
