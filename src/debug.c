//
//  debug.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#include "debug.h"

#include "bus.h"
#include "bytes.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// X(symbol, description)
#define DEBUG_STATE_X \
X(DBG_INACTIVE, "Inactive") \
X(DBG_RUN, "Run") \
X(DBG_BREAK, "Break")

enum debugger_state {
#define X(s, d) s,
    DEBUG_STATE_X
#undef X
};

struct resdecorator {
    struct busdevice inner;
    uint16_t vector;
};

enum breakpointstatus {
    BPS_FREE,
    BPS_DISABLED,
    BPS_ENABLED,
};

struct breakpoint {
    struct haltexpr expr;
    enum breakpointstatus status;
};

struct breakpoint_vector {
    size_t capacity, size;
    struct breakpoint *items;
};

struct debugger_context {
    struct mos6502 *cpu;    // Non-owning Pointer
    struct resdecorator *dec;
    struct breakpoint_vector breakpoints;
    enum debugger_state state;
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

static struct breakpoint *bpvector_find_slot(struct breakpoint_vector *vec)
{
    for (size_t i = 0; i < vec->capacity; ++i) {
        if (vec->items[i].status == BPS_FREE) {
            return vec->items + i;
        }
    }
    return NULL;
}

static void bpvector_resize(struct breakpoint_vector *vec)
{
    static const double k = 1.5;
    vec->capacity *= k;
    vec->items = realloc(vec->items, vec->capacity * sizeof *vec->items);
    assert(vec->items != NULL);
    for (size_t i = vec->size; i < vec->capacity; ++i) {
        vec->items[i] = (struct breakpoint){0};
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
    ++vec->size;

    assert(vec->size <= vec->capacity);
}

static void bpvector_free(struct breakpoint_vector *vec)
{
    free(vec->items);
}

//
// Breakpoints
//

static bool bp_address(const struct debugger_context *self)
{
    // TODO: revive this once breakpoints are created
    //return self->cpu->signal.sync && self->cpu->addrinst == self->haltaddr;
    (void)self;
    return false;
}

//
// Public Interface
//

debugctx *debug_new(const struct control *appstate)
{
    assert(appstate != NULL);

    struct debugger_context *const self = malloc(sizeof *self);
    *self = (struct debugger_context){
        .resetvector = appstate->resetvector,
        .state = DBG_INACTIVE,
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

void debug_attach_cpu(debugctx *self, struct mos6502 *cpu)
{
    assert(self != NULL);
    assert(cpu != NULL);

    self->cpu = cpu;
}

void debug_detach(debugctx *self)
{
    self->cpu = NULL;
}

void debug_override_reset(debugctx *self, uint16_t device_addr)
{
    assert(self != NULL);

    if (self->resetvector < 0 || !self->cpu) return;

    self->dec = malloc(sizeof *self->dec);
    self->dec->vector = self->resetvector;
    struct busdevice resetaddr_device = {
        resetaddr_read, resetaddr_write, resetaddr_dma, self->dec,
    };
    bus_swap(self->cpu->bus, device_addr, resetaddr_device, &self->dec->inner);
}

void debug_addbreakpoint(debugctx *self, struct haltexpr expr)
{
    assert(self != NULL);

    bpvector_insert(&self->breakpoints, expr);
}

void debug_check(debugctx *self)
{
    assert(self != NULL);

    if (!self->cpu) return;

    switch (self->state) {
    case DBG_RUN:
        if (bp_address(self)) {
            self->cpu->signal.rdy = false;
            self->state = DBG_BREAK;
        }
        break;
    case DBG_BREAK:
        self->state = DBG_RUN;
        break;
    default:
        break;
    }
}

const char *debug_description(int state)
{
    switch (state) {
#define X(s, d) case s: return d;
        DEBUG_STATE_X
#undef X
    default:
        return "Invalid";
    }
}

void debug_snapshot(debugctx *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->debugger.halted = self->state == DBG_BREAK;
    snapshot->debugger.resvector_override = self->resetvector;
    snapshot->debugger.state = self->state;
}
