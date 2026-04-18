//
//  console.h
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#ifndef Aldo_console_h
#define Aldo_console_h

#include "cart.h"
#include "ctrlsignal.h"
#include "debug.h"

#include <stddef.h>
#include <stdio.h>

// X(symbol, name)
#define ALDO_CONSOLE_TYPE_X \
X(CONSOLE_ALDO8, "Aldo8") \
X(CONSOLE_NES, "NES")

enum aldo_console_type {
#define X(s, n) ALDO_##s,
    ALDO_CONSOLE_TYPE_X
#undef X
};

struct aldo_clock;
struct aldo_snapshot;

// Public ADT for Console Emulation
typedef struct aldo_console_base aldo_console;

#include "bridgeopen.h"
// if returns false then errno is set due to failed allocation and *cn is unchanged
aldo_export aldo_checkerr
bool aldo_console_poweron(aldo_console **cn, aldo_cart *c, aldo_debugger *dbg,
                         FILE *tracelog, bool zeroram) aldo_nothrow;
aldo_export
void aldo_console_free(aldo_console *self) aldo_nothrow;

aldo_export
int aldo_console_max_tcpu() aldo_nothrow;
aldo_export
enum aldo_console_type aldo_console_type(aldo_console *self) aldo_nothrow;
aldo_export
const char *aldo_console_name(aldo_console *self) aldo_nothrow;
aldo_export
size_t aldo_console_ram_size(aldo_console *self) aldo_nothrow;
aldo_export
void aldo_console_screen_size(aldo_console *self, int *width,
                              int *height) aldo_nothrow;
aldo_export
int aldo_console_cycle_factor(aldo_console *self) aldo_nothrow;
aldo_export
int aldo_console_frame_factor(aldo_console *self) aldo_nothrow;
aldo_export
bool aldo_console_bcd_support(aldo_console *self) aldo_nothrow;
aldo_export
bool aldo_console_tracefailed(aldo_console *self) aldo_nothrow;
aldo_export
enum aldo_execmode aldo_console_mode(aldo_console *self) aldo_nothrow;
aldo_export
void aldo_console_set_mode(aldo_console *self,
                           enum aldo_execmode mode) aldo_nothrow;
aldo_export
bool aldo_console_halted(aldo_console *self) aldo_nothrow;
aldo_export
void aldo_console_halt(aldo_console *self, bool halt) aldo_nothrow;
aldo_export
bool aldo_console_probe(aldo_console *self,
                        enum aldo_interrupt signal) aldo_nothrow;
aldo_export
void aldo_console_set_probe(aldo_console *self, enum aldo_interrupt signal,
                            bool active) aldo_nothrow;

aldo_export
void aldo_console_clock(aldo_console *self,
                        struct aldo_clock *clock) aldo_nothrow;

aldo_export
const struct aldo_snapshot *aldo_console_snapshot(aldo_console *self) aldo_nothrow;
aldo_export
size_t aldo_console_dumpcount(aldo_console *self) aldo_nothrow;
aldo_export
void aldo_console_dumpram(aldo_console *self, size_t count, FILE *fs[aldo_sz(count)],
                          bool errs[aldo_sz(count)]) aldo_nothrow;
#include "bridgeclose.h"

#endif
