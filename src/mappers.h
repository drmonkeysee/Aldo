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
#include <stdlib.h>

struct mapper;
typedef bool busconn(struct mapper *, bus *);
typedef void busdisconn(const struct mapper *, bus *);
typedef const uint8_t *mapper_rom(const struct mapper *);
typedef size_t bankord(const struct mapper *);

struct mapper {
    void (*dtor)(struct mapper *);
    busconn *mbus_connect;
    busdisconn *mbus_disconnect;
    mapper_rom *prgrom;
    bankord *currprg;
};

struct nesmapper {
    struct mapper extends;
    busconn *vbus_connect;
    busdisconn *vbus_disconnect;
    mapper_rom *chrrom;
    bankord *currchr;
};

// NOTE: if create functions return non-zero error code, *m is unmodified
int mapper_raw_create(struct mapper **m, FILE *f);
int mapper_ines_create(struct mapper **m, struct ines_header *header, FILE *f);

#endif
