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
#include <stdint.h>
#include <stdio.h>

struct mapper;
typedef bool mapper_connect(struct mapper *, bus *);
typedef void mapper_disconnect(const struct mapper *, bus *);
typedef const uint8_t *mapper_mem(const struct mapper *);

struct mapper {
    void (*dtor)(struct mapper *);
    mapper_connect *mbus_connect;
    mapper_disconnect *mbus_disconnect;
    mapper_mem *prgrom;

    // NOTE: optional protocol
    mapper_connect *vbus_connect;
    mapper_disconnect *vbus_disconnect;
    mapper_mem *chrrom;
};

// NOTE: if create functions return non-zero error code, *m is unmodified
int mapper_raw_create(struct mapper **m, FILE *f);
int mapper_ines_create(struct mapper **m, struct ines_header *header, FILE *f);

#endif
