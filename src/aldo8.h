//
//  aldo8.h
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#ifndef Aldo_aldo8_h
#define Aldo_aldo8_h

#include "bytes.h"
#include "consoledef.h"
#include "cpu.h"
#include "debug.h"

#include <stddef.h>
#include <stdint.h>

struct aldo_clock;
struct aldo_snapshot;

// The Aldo-8 fictional 8-bit console, useful for testing and tinkering with
// emulating a generic 6502-based system.
struct aldo_aldo8 {
    struct aldo_console_base extends;

    uint8_t ram[ALDO_MEMBLOCK_32KB];    // Internal RAM
};

bool aldo_aldo8_init(struct aldo_aldo8 *self);

void aldo_aldo8_powerup(struct aldo_aldo8 *self, bool zeroram);

size_t aldo_aldo8_ram_size(const struct aldo_aldo8 *self);

void aldo_aldo8_clock(struct aldo_aldo8 *self, struct aldo_clock *clock);

void aldo_aldo8_snapshot_core(const struct aldo_aldo8 *self,
                              struct aldo_snapshot *snp);

#endif
