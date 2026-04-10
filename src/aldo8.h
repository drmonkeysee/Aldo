//
//  aldo8.h
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#ifndef Aldo_aldo8_h
#define Aldo_aldo8_h

#include "debug.h"

#include <stdio.h>

typedef struct aldo_aldo8_console aldo_aldo8;

// TODO: figure out lib bridge later with console base

aldo_aldo8 *aldo_aldo8_new(aldo_debugger *dbg, FILE *tracelog);
void aldo_aldo8_free(aldo_aldo8 *self);

#endif
