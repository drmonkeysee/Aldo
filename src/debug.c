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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum debugger_state {
    DBG_INACTIVE,
    DBG_RUN,
    DBG_BREAK,
};

struct resdecorator {
    struct busdevice inner;
    uint16_t vector;
};

struct debugger_context {
    struct mos6502 *cpu;    // Non-owning Pointer
    struct resdecorator *dec;
    enum debugger_state state;
    int resetvector, haltaddr;
};

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
// Public Interface
//

debugctx *debug_new(const struct control *appstate)
{
    assert(appstate != NULL);

    struct debugger_context *const self = malloc(sizeof *self);
    *self = (struct debugger_context){
        .haltaddr = appstate->haltaddr,
        .resetvector = appstate->resetvector,
    };
    return self;
}

void debug_free(debugctx *self)
{
    assert(self != NULL);

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

void debug_check(debugctx *self)
{
    assert(self != NULL);

    if (self->state == DBG_INACTIVE) return;
}

void debug_snapshot(debugctx *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->mem.resvector_override = self->resetvector;
}
