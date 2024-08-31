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
#include "snapshot.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct mapper;
typedef bool busconn(struct mapper *, bus *);
typedef void busdisconn(bus *);
typedef const uint8_t *mapper_rom(const struct mapper *);

struct mapper {
    void (*dtor)(struct mapper *);
    busconn *mbus_connect;
    busdisconn *mbus_disconnect;
    mapper_rom *prgrom;
};

struct nesmapper {
    struct mapper extends;
    busconn *vbus_connect;
    busdisconn *vbus_disconnect;
    mapper_rom *chrrom;
    // Optional Interface
    void (*snapshot)(const struct mapper *, struct snapshot *);
};

// NOTE: if create functions return non-zero error code, *m is unmodified
int mapper_raw_create(struct mapper **m, FILE *f);
int mapper_ines_create(struct mapper **m, struct ines_header *header, FILE *f);

#endif
