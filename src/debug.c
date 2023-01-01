//
//  debug.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#include "debug.h"

#include "bytes.h"
#include "tsutil.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static const ptrdiff_t NoBreakpoint = -1;

struct debugger_context {
    struct breakpoint_vector {
        size_t capacity, size;
        struct breakpoint *items;
    } breakpoints;
    struct mos6502 *cpu;    // Non-owning Pointer
    struct resdecorator {
        struct busdevice inner;
        uint16_t vector;
        bool active;
    } dec;
    ptrdiff_t halted;
    int resetvector;
};

static void update_reset_override(struct debugger_context *self)
{
    if (self->resetvector == NoResetVector) {
        debug_remove_reset_override(self);
    } else {
        debug_add_reset_override(self);
    }
}

//
// Bus Interception
//

static bool resetaddr_read(const void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    const struct resdecorator *const dec = ctx;
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
    struct resdecorator *const dec = ctx;
    return dec->inner.write
            ? dec->inner.write(dec->inner.ctx, addr, d)
            : false;
}

static size_t resetaddr_dma(const void *restrict ctx, uint16_t addr,
                            size_t count, uint8_t dest[restrict count])
{
    const struct resdecorator *const dec = ctx;
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
    return isgreaterequal(clk->runtime - bp->expr.runtime, 1.0 / TSU_MS_PER_S);
}

static bool halt_cycles(const struct breakpoint *bp,
                        const struct cycleclock *clk)
{
    return clk->total_cycles == bp->expr.cycles;
}

static bool halt_jammed(const struct mos6502 *cpu)
{
    return cpu_jammed(cpu);
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

static struct breakpoint *bpvector_at(const struct breakpoint_vector *vec,
                                      ptrdiff_t at)
{
    if (at < 0 || at >= (ptrdiff_t)vec->size) return NULL;
    return vec->items + at;
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
    if (vec->size == vec->capacity) {
        bpvector_resize(vec);
    }
    struct breakpoint *const slot = vec->items + vec->size;
    *slot = (struct breakpoint){expr, BPS_ENABLED};
    ++vec->size;
}

static ptrdiff_t bpvector_break(const struct breakpoint_vector *vec,
                                const struct cycleclock *clk,
                                const struct mos6502 *cpu)
{
    for (ptrdiff_t i = 0; i < (ptrdiff_t)vec->size; ++i) {
        const struct breakpoint *const bp = vec->items + i;
        if (bp->status != BPS_ENABLED) continue;
        switch (bp->expr.cond) {
        case HLT_ADDR:
            if (halt_address(bp, cpu)) return i;
            break;
        case HLT_TIME:
            if (halt_runtime(bp, clk)) return i;
            break;
        case HLT_CYCLES:
            if (halt_cycles(bp, clk)) return i;
            break;
        case HLT_JAM:
            if (halt_jammed(cpu)) return i;
            break;
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

const int NoResetVector = -1;

debugctx *debug_new(void)
{
    struct debugger_context *const self = malloc(sizeof *self);
    *self = (struct debugger_context){
        .halted = NoBreakpoint,
        .resetvector = NoResetVector,
    };
    bpvector_init(&self->breakpoints);
    return self;
}

void debug_free(debugctx *self)
{
    assert(self != NULL);

    bpvector_free(&self->breakpoints);
    free(self);
}

void debug_set_resetvector(debugctx *self, int resetvector)
{
    assert(self != NULL);

    self->resetvector = resetvector;
    update_reset_override(self);
}

void debug_cpu_connect(debugctx *self, struct mos6502 *cpu)
{
    assert(self != NULL);
    assert(cpu != NULL);

    self->cpu = cpu;
}

void debug_cpu_disconnect(debugctx *self)
{
    assert(self != NULL);

    self->cpu = NULL;
}

void debug_add_reset_override(debugctx *self)
{
    assert(self != NULL);

    if (self->resetvector == NoResetVector || !self->cpu) return;

    if (self->dec.active) {
        self->dec.vector = (uint16_t)self->resetvector;
        return;
    }

    self->dec = (struct resdecorator){.vector = (uint16_t)self->resetvector};
    struct busdevice resetaddr_device = {
        resetaddr_read, resetaddr_write, resetaddr_dma, &self->dec,
    };
    self->dec.active = bus_swap(self->cpu->bus, CPU_VECTOR_RES,
                                resetaddr_device, &self->dec.inner);
}

void debug_remove_reset_override(debugctx *self)
{
    assert(self != NULL);

    if (!self->dec.active) return;

    bus_set(self->cpu->bus, CPU_VECTOR_RES, self->dec.inner);
    self->dec = (struct resdecorator){0};
}

void debug_bp_add(debugctx *self, struct haltexpr expr)
{
    assert(self != NULL);
    assert(HLT_NONE < expr.cond && expr.cond < HLT_CONDCOUNT);

    bpvector_insert(&self->breakpoints, expr);
}

const struct breakpoint *debug_bp_at(debugctx *self, ptrdiff_t at)
{
    assert(self != NULL);

    return bpvector_at(&self->breakpoints, at);
}

size_t debug_bp_count(debugctx *self)
{
    return self->breakpoints.size;
}

void debug_check(debugctx *self, const struct cycleclock *clk)
{
    assert(self != NULL);
    assert(clk != NULL);

    if (!self->cpu) return;

    if (self->halted == NoBreakpoint
        && (self->halted = bpvector_break(&self->breakpoints, clk, self->cpu))
            != NoBreakpoint) {
        self->cpu->signal.rdy = false;
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
