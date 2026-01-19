//
//  mappers.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/22/21.
//

#ifndef Aldo_mappers_h
#define Aldo_mappers_h

#include "bustype.h"
#include "cart.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct aldo_mapper;
struct aldo_snapshot;
typedef bool aldo_busconn(struct aldo_mapper *, aldo_bus *);
typedef void aldo_busdisconn(aldo_bus *);
typedef const uint8_t *aldo_mapper_rom(const struct aldo_mapper *);

struct aldo_mapper {
    void (*dtor)(struct aldo_mapper *);
    aldo_busconn *mbus_connect;
    aldo_busdisconn *mbus_disconnect;
    aldo_mapper_rom *prgrom;
};

struct aldo_nesmapper {
    struct aldo_mapper extends;
    aldo_busconn *vbus_connect;
    aldo_busdisconn *vbus_disconnect;
    aldo_mapper_rom *chrrom;
    // Optional Interface
    void (*snapshot)(struct aldo_mapper *, struct aldo_snapshot *);
};

// if create functions return non-zero error code, *m is unmodified
int aldo_mapper_raw_create(struct aldo_mapper **m, FILE *f);
int aldo_mapper_ines_create(struct aldo_mapper **m,
                            struct aldo_ines_header *header, FILE *f);

#endif
