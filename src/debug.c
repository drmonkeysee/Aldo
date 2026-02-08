//
//  debug.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/21.
//

#include "debug.h"

#include "bus.h"
#include "bytes.h"
#include "cpu.h"
#include "cycleclock.h"
#include "tsutil.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct aldo_debugger_context {
    struct breakpoint_vector {
        size_t capacity, size;
        struct aldo_breakpoint *items;
    } breakpoints;
    struct aldo_mos6502 *cpu;   // Non-owning Pointer
    struct resdecorator {
        struct aldo_busdevice inner;
        uint16_t vector;
        bool active;
    } dec;
    ptrdiff_t halted;
    int resetvector;
};

static void remove_reset_override(struct aldo_debugger_context *self)
{
    if (!self->dec.active) return;

    auto r = aldo_bus_set(self->cpu->mbus, ALDO_CPU_VECTOR_RST,
                          self->dec.inner);
    (void)r, assert(r);
    self->dec = (typeof(self->dec)){};
}

static void update_reset_override(struct aldo_debugger_context *self)
{
    if (self->resetvector == Aldo_NoResetVector) {
        remove_reset_override(self);
    } else {
        aldo_debug_sync_bus(self);
    }
}

//
// MARK: - Bus Interception
//

static bool resetaddr_read(void *restrict ctx, uint16_t addr,
                           uint8_t *restrict d)
{
    const struct resdecorator *dec = ctx;
    if (!dec->inner.read) return false;

    if (ALDO_CPU_VECTOR_RST <= addr && addr < ALDO_CPU_VECTOR_IRQ) {
        uint8_t vector[2];
        aldo_wrtoba(dec->vector, vector);
        *d = vector[addr - ALDO_CPU_VECTOR_RST];
        return true;
    }
    return dec->inner.read(dec->inner.ctx, addr, d);
}

static bool resetaddr_write(void *ctx, uint16_t addr, uint8_t d)
{
    struct resdecorator *dec = ctx;
    return dec->inner.write
            ? dec->inner.write(dec->inner.ctx, addr, d)
            : false;
}

static size_t resetaddr_copy(const void *restrict ctx, uint16_t addr,
                             size_t count, uint8_t dest[restrict count])
{
    const struct resdecorator *dec = ctx;
    return dec->inner.copy
            ? dec->inner.copy(dec->inner.ctx, addr, count, dest)
            : 0;
}

//
// MARK: - Breakpoints
//

static bool halt_address(const struct aldo_breakpoint *bp,
                         const struct aldo_mos6502 *cpu)
{
    return cpu->signal.sync && cpu->addrinst == bp->expr.address;
}

static bool halt_runtime(const struct aldo_breakpoint *bp,
                         const struct aldo_clock *clk)
{
    return isgreaterequal(clk->emutime - bp->expr.runtime,
                          1.0 / ALDO_MS_PER_S);
}

static bool halt_cycles(const struct aldo_breakpoint *bp,
                        const struct aldo_clock *clk)
{
    return clk->cycles == bp->expr.cycles;
}

static bool halt_frames(const struct aldo_breakpoint *bp,
                        const struct aldo_clock *clk)
{
    return clk->frames == bp->expr.frames;
}

static bool halt_jammed(const struct aldo_mos6502 *cpu)
{
    return aldo_cpu_jammed(cpu);
}

//
// MARK: - Breakpoint Vector
//

static bool bpvector_valid_index(const struct breakpoint_vector *vec,
                                 ptrdiff_t at)
{
    return 0 <= at && at < (ptrdiff_t)vec->size;
}

static bool bpvector_init(struct breakpoint_vector *vec)
{
    static constexpr size_t initial_capacity = 2;
    *vec = (typeof(*vec)){
        .capacity = initial_capacity,
        .items = calloc(initial_capacity, sizeof *vec->items),
    };
    return vec->items != nullptr;
}

static struct aldo_breakpoint *bpvector_at(const struct breakpoint_vector *vec,
                                           ptrdiff_t at)
{
    if (bpvector_valid_index(vec, at)) return vec->items + at;
    return nullptr;
}

static bool bpvector_resize(struct breakpoint_vector *vec)
{
    // growth factor K = 1.5
    vec->capacity += vec->capacity / 2;
    struct aldo_breakpoint *bpv = realloc(vec->items,
                                          vec->capacity * sizeof *vec->items);
    if (bpv) {
        vec->items = bpv;
    }
    return bpv != nullptr;
}

static bool bpvector_insert(struct breakpoint_vector *vec,
                            struct aldo_haltexpr expr)
{
    if (vec->size == vec->capacity) {
        if (!bpvector_resize(vec)) return false;
    }
    auto slot = vec->items + vec->size;
    *slot = (typeof(*slot)){expr, true};
    ++vec->size;
    return true;
}

static ptrdiff_t bpvector_break(const struct breakpoint_vector *vec,
                                const struct aldo_clock *clk,
                                const struct aldo_mos6502 *cpu)
{
    for (ptrdiff_t i = 0; i < (ptrdiff_t)vec->size; ++i) {
        auto bp = vec->items + i;
        if (!bp->enabled) continue;
        switch (bp->expr.cond) {
        case ALDO_HLT_ADDR:
            if (halt_address(bp, cpu)) return i;
            break;
        case ALDO_HLT_TIME:
            if (halt_runtime(bp, clk)) return i;
            break;
        case ALDO_HLT_CYCLES:
            if (halt_cycles(bp, clk)) return i;
            break;
        case ALDO_HLT_FRAMES:
            if (halt_frames(bp, clk)) return i;
            break;
        case ALDO_HLT_JAM:
            if (halt_jammed(cpu)) return i;
            break;
        default:
            break;
        }
    }
    return Aldo_NoBreakpoint;
}

static bool bpvector_remove(struct breakpoint_vector *vec, ptrdiff_t at)
{
    if (!bpvector_valid_index(vec, at)) return false;

    if (at < (ptrdiff_t)vec->size - 1) {
        const struct aldo_breakpoint
            *rest = vec->items + at + 1,
            *end = vec->items + vec->size;
        ptrdiff_t count = end - rest;
        assert(count > 0);
        memmove(vec->items + at, rest, sizeof *vec->items * (size_t)count);
    }
    --vec->size;
    return true;
}

static void bpvector_clear(struct breakpoint_vector *vec)
{
    vec->size = 0;
}

static void bpvector_free(struct breakpoint_vector *vec)
{
    free(vec->items);
}

//
// MARK: - Public Interface
//

const int Aldo_NoResetVector = -1;
const ptrdiff_t Aldo_NoBreakpoint = -1;

aldo_debugger *aldo_debug_new()
{
    struct aldo_debugger_context *self = malloc(sizeof *self);
    if (!self) return self;

    *self = (typeof(*self)){
        .halted = Aldo_NoBreakpoint,
        .resetvector = Aldo_NoResetVector,
    };
    if (!bpvector_init(&self->breakpoints)) {
        aldo_debug_free(self);
        return nullptr;
    }
    return self;
}

void aldo_debug_free(aldo_debugger *self)
{
    assert(self != nullptr);

    bpvector_free(&self->breakpoints);
    free(self);
}

int aldo_debug_vector_override(aldo_debugger *self)
{
    assert(self != nullptr);

    return self->resetvector;
}

void aldo_debug_set_vector_override(aldo_debugger *self, int resetvector)
{
    assert(self != nullptr);

    self->resetvector = resetvector;
    update_reset_override(self);
}

bool aldo_debug_bp_add(aldo_debugger *self, struct aldo_haltexpr expr)
{
    assert(self != nullptr);
    assert(ALDO_HLT_NONE < expr.cond && expr.cond < ALDO_HLT_COUNT);

    return bpvector_insert(&self->breakpoints, expr);
}

const struct aldo_breakpoint *aldo_debug_bp_at(aldo_debugger *self,
                                               ptrdiff_t at)
{
    assert(self != nullptr);

    return bpvector_at(&self->breakpoints, at);
}

void aldo_debug_bp_enable(aldo_debugger *self, ptrdiff_t at, bool enabled)
{
    assert(self != nullptr);

    auto bp = bpvector_at(&self->breakpoints, at);
    if (bp) {
        bp->enabled = enabled;
    }
}

const struct aldo_breakpoint *aldo_debug_halted(aldo_debugger *self)
{
    assert(self != nullptr);

    return self->halted == Aldo_NoBreakpoint
            ? nullptr
            : aldo_debug_bp_at(self, self->halted);
}

ptrdiff_t aldo_debug_halted_at(aldo_debugger *self)
{
    assert(self != nullptr);

    return self->halted;
}

void aldo_debug_bp_remove(aldo_debugger *self, ptrdiff_t at)
{
    assert(self != nullptr);

    if (!bpvector_remove(&self->breakpoints, at)) return;

    // If we removed the currently halted breakpoint we need to clear
    // the halt flag; if we removed a breakpoint before the currently halted
    // breakpoint we need to adjust the flag back one.
    if (self->halted == at) {
        self->halted = Aldo_NoBreakpoint;
    } else if (self->halted > at) {
        --self->halted;
    }
}

void aldo_debug_bp_clear(aldo_debugger *self)
{
    assert(self != nullptr);

    bpvector_clear(&self->breakpoints);
    self->halted = Aldo_NoBreakpoint;
}

size_t aldo_debug_bp_count(aldo_debugger *self)
{
    assert(self != nullptr);

    return self->breakpoints.size;
}

void aldo_debug_reset(aldo_debugger *self)
{
    assert(self != nullptr);

    aldo_debug_set_vector_override(self, Aldo_NoResetVector);
    aldo_debug_bp_clear(self);
}

//
// MARK: - Internal Interface
//

void aldo_debug_cpu_connect(aldo_debugger *self, struct aldo_mos6502 *cpu)
{
    assert(self != nullptr);
    assert(cpu != nullptr);

    self->cpu = cpu;
}

void aldo_debug_cpu_disconnect(aldo_debugger *self)
{
    assert(self != nullptr);

    self->cpu = nullptr;
}

void aldo_debug_sync_bus(aldo_debugger *self)
{
    assert(self != nullptr);

    if (self->resetvector == Aldo_NoResetVector || !self->cpu) return;

    if (self->dec.active) {
        self->dec.vector = (uint16_t)self->resetvector;
        return;
    }

    self->dec = (typeof(self->dec)){.vector = (uint16_t)self->resetvector};
    struct aldo_busdevice resetaddr_device = {
        resetaddr_read, resetaddr_write, resetaddr_copy, &self->dec,
    };
    self->dec.active = aldo_bus_swap(self->cpu->mbus, ALDO_CPU_VECTOR_RST,
                                     resetaddr_device, &self->dec.inner);
}

void aldo_debug_check(aldo_debugger *self, const struct aldo_clock *clk)
{
    assert(self != nullptr);
    assert(clk != nullptr);

    if (!self->cpu) return;

    if (self->halted == Aldo_NoBreakpoint
        && (self->halted = bpvector_break(&self->breakpoints, clk, self->cpu))
            != Aldo_NoBreakpoint) {
        self->cpu->signal.rdy = false;
    } else {
        self->halted = Aldo_NoBreakpoint;
    }
}
