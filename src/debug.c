//
//  debug.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#include "debug.h"

#include "bytes.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct resdecorator {
    struct busdevice inner;
    uint16_t vector;
};

enum breakpointstatus {
    BPS_FREE,
    BPS_DISABLED,
    BPS_ENABLED,
};

typedef ptrdiff_t bphandle;
static const bphandle NoBreakpoint = -1;

struct breakpoint {
    struct haltexpr expr;
    enum breakpointstatus status;
};

struct breakpoint_vector {
    size_t capacity;
    struct breakpoint *items;
};

struct debugger_context {
    struct resdecorator *dec;
    struct breakpoint_vector breakpoints;
    bphandle halted;
    int resetvector;
};

//
// Bus Interception
//

static bool resetaddr_read(const void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    struct resdecorator *const dec = (struct resdecorator *)ctx;
    if (!dec->inner.read) return false;

    if (CPU_VECTOR_RES <= addr && addr < CPU_VECTOR_IRQ) {
        uint8_t vector[2];
        wrtoba(dec->vector, vector);
        *d = vector[addr - CPU_VECTOR_RES];
        return true;
    }
    return dec->inner.read(dec->inner.ctx, addr, d);
}

static bool resetaddr_write(void *ctx, uint16_t addr, uint8_t d)
{
    struct resdecorator *const dec = (struct resdecorator *)ctx;
    return dec->inner.write
            ? dec->inner.write(dec->inner.ctx, addr, d)
            : false;
}

static size_t resetaddr_dma(const void *restrict ctx, uint16_t addr,
                            size_t count, uint8_t dest[restrict count])
{
    struct resdecorator *const dec = (struct resdecorator *)ctx;
    return dec->inner.dma
            ? dec->inner.dma(dec->inner.ctx, addr, count, dest)
            : 0;
}

//
// Breakpoints
//

static bool halt_address(const struct breakpoint *bp,
                         const struct mos6502 *cpu)
{
    return cpu->signal.sync && cpu->addrinst == bp->expr.address;
}

static bool halt_runtime(const struct breakpoint *bp,
                         const struct cycleclock *clk)
{
    // TODO: implement this
    return false;
}

static bool halt_cycles(const struct breakpoint *bp,
                        const struct cycleclock *clk)
{
    return clk->total_cycles == bp->expr.cycles;
}

//
// Breakpoint Vector
//

static void bpvector_init(struct breakpoint_vector *vec)
{
    static const size_t initial_capacity = 2;
    *vec = (struct breakpoint_vector){
        .capacity = initial_capacity,
        .items = calloc(initial_capacity, sizeof *vec->items),
    };
}

static struct breakpoint *
bpvector_find_slot(const struct breakpoint_vector *vec)
{
    for (size_t i = 0; i < vec->capacity; ++i) {
        if (vec->items[i].status == BPS_FREE) return vec->items + i;
    }
    return NULL;
}

static struct breakpoint *bpvector_at(const struct breakpoint_vector *vec,
                                      bphandle h)
{
    assert(0 <= h && h < (bphandle)vec->capacity);

    return vec->items + h;
}

static void bpvector_resize(struct breakpoint_vector *vec)
{
    const size_t oldcap = vec->capacity;
    // NOTE: growth factor K = 1.5
    vec->capacity += vec->capacity / 2;
    vec->items = realloc(vec->items, vec->capacity * sizeof *vec->items);
    assert(vec->items != NULL);
    for (size_t i = oldcap; i < vec->capacity; ++i) {
        vec->items[i] = (struct breakpoint){.status = BPS_FREE};
    }
}

static void bpvector_insert(struct breakpoint_vector *vec,
                            struct haltexpr expr)
{
    struct breakpoint *slot = bpvector_find_slot(vec);
    if (!slot) {
        bpvector_resize(vec);
        slot = bpvector_find_slot(vec);
    }
    assert(slot != NULL);
    *slot = (struct breakpoint){expr, BPS_ENABLED};
}

static bphandle bpvector_break(const struct breakpoint_vector *vec,
                               const struct cycleclock *clk,
                               const struct mos6502 *cpu)
{
    for (size_t i = 0; i < vec->capacity; ++i) {
        const struct breakpoint *const bp = vec->items + i;
        if (bp->status != BPS_ENABLED) continue;
        switch (bp->expr.cond) {
        case HLT_ADDR:
            if (halt_address(bp, cpu)) return i;
        case HLT_TIME:
            if (halt_runtime(bp, clk)) return i;
        case HLT_CYCLES:
            if (halt_cycles(bp, clk)) return i;
        default:
            break;
        }
    }
    return NoBreakpoint;
}

static void bpvector_free(struct breakpoint_vector *vec)
{
    free(vec->items);
    vec->items = NULL;
}

//
// Public Interface
//

debugctx *debug_new(const struct control *appstate)
{
    assert(appstate != NULL);

    struct debugger_context *const self = malloc(sizeof *self);
    *self = (struct debugger_context){
        .halted = NoBreakpoint,
        .resetvector = appstate->resetvector,
    };
    bpvector_init(&self->breakpoints);
    return self;
}

void debug_free(debugctx *self)
{
    assert(self != NULL);

    bpvector_free(&self->breakpoints);
    free(self->dec);
    free(self);
}

void debug_override_reset(debugctx *self, bus *b, uint16_t device_addr)
{
    assert(self != NULL);
    assert(b != NULL);

    if (self->resetvector < 0) return;

    self->dec = malloc(sizeof *self->dec);
    self->dec->vector = self->resetvector;
    struct busdevice resetaddr_device = {
        resetaddr_read, resetaddr_write, resetaddr_dma, self->dec,
    };
    bus_swap(b, device_addr, resetaddr_device, &self->dec->inner);
}

void debug_addbreakpoint(debugctx *self, struct haltexpr expr)
{
    assert(self != NULL);

    bpvector_insert(&self->breakpoints, expr);
}

void debug_check(debugctx *self, const struct cycleclock *clk,
                 struct mos6502 *cpu)
{
    assert(self != NULL);
    assert(clk != NULL);
    assert(cpu != NULL);

    if (self->halted == NoBreakpoint
        && (self->halted = bpvector_break(&self->breakpoints, clk, cpu))
            != NoBreakpoint) {
        cpu->signal.rdy = false;
    } else {
        self->halted = NoBreakpoint;
    }
}

void debug_snapshot(debugctx *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->debugger.break_condition = self->halted == NoBreakpoint
                                            ? (struct haltexpr){
                                                .cond = HLT_NONE,
                                            }
                                            : bpvector_at(&self->breakpoints,
                                                          self->halted)->expr;
    snapshot->debugger.resvector_override = self->resetvector;
}
