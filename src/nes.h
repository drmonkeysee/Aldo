//
//  nes.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#ifndef Aldo_nes_h
#define Aldo_nes_h

#include "console.h"
#include "ctrlsignal.h"

#include <stddef.h>
#include <stdio.h>

struct aldo_clock;
typedef struct aldo_nes001 aldo_nes;

extern const size_t Aldo_NesSize;

bool aldo_nes_init(aldo_nes *self);
void aldo_nes_powerup(aldo_nes *self, bool zeroram);

bool aldo_nes_clock(aldo_nes *self, struct aldo_clock *clock);
enum aldo_trivalue aldo_nes_probe(aldo_nes *self, enum aldo_probe probe);
void aldo_nes_set_probe(aldo_nes *self, enum aldo_probe probe,
                        enum aldo_trivalue val);
void aldo_nes_snapshot_init(aldo_nes *self);
void aldo_nes_snapshot_core(aldo_nes *self);

// TODO: remove this when no longer needed
void aldo_nes_screen_size(int *width, int *height);

void aldo_nes_dumpram(aldo_nes *self, FILE *fs[static 3], bool errs[static 3]);

#endif
