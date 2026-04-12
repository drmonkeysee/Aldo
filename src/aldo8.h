//
//  aldo8.h
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#ifndef Aldo_aldo8_h
#define Aldo_aldo8_h

#include <stddef.h>

struct aldo_clock;
struct aldo_snapshot;
typedef struct aldo_aldo8_console aldo_aldo8;

extern const size_t Aldo_Aldo8Size;

bool aldo_aldo8_init(aldo_aldo8 *self);
void aldo_aldo8_powerup(aldo_aldo8 *self, bool zeroram);

void aldo_aldo8_clock(aldo_aldo8 *self, struct aldo_clock *clock);
void aldo_aldo8_snapshot_core(aldo_aldo8 *self, struct aldo_snapshot *snp);

#endif
