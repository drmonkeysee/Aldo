//
//  cpu.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#include "cpu.h"

#include "traits.h"

#include <assert.h>
#include <stddef.h>

static bool read(const struct mos6502 *self, uint16_t addr,
                 uint8_t *restrict data)
{
    if (addr < RAM_SIZE) {
        *data = self->ram[addr];
        return true;
    }
    if (CART_MIN_CPU_ADDR <= addr && addr <= CART_MAX_CPU_ADDR) {
        *data = self->cart[addr & CART_CPU_ADDR_MASK];
        return true;
    }
    return false;
}

static void reset_pc(struct mos6502 *self)
{
    // TODO: what to do if read returns false?
    uint8_t lo, hi;
    read(self, RESET_VECTOR, &lo);
    read(self, RESET_VECTOR + 1, &hi);
    self->pc = (hi << 8) | lo;
}

static uint8_t get_p(const struct mos6502 *self)
{
    uint8_t p = 0;
    p |= self->p.c << 0;
    p |= self->p.z << 1;
    p |= self->p.i << 2;
    p |= self->p.d << 3;
    p |= self->p.b << 4;
    p |= self->p.u << 5;
    p |= self->p.v << 6;
    p |= self->p.n << 7;
    return p;
}

static void set_p(struct mos6502 *self, uint8_t b)
{
    self->p.c = b & 0x1;
    self->p.z = b & 0x2;
    self->p.i = b & 0x4;
    self->p.d = b & 0x8;
    self->p.b = b & 0x10;
    self->p.u = b & 0x20;
    self->p.v = b & 0x40;
    self->p.n = b & 0x80;
}

//
// Public Interface
//

void cpu_reset(struct mos6502 *self)
{
    // TODO: this will consume 7 cycles
    assert(self != NULL);

    reset_pc(self);
    set_p(self, 0x34);
    self->a = self->x = self->y = 0;
    self->s = 0xfd;
}

void cpu_snapshot(const struct mos6502 *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->program_counter = self->pc;
    snapshot->stack_pointer = self->s;
    snapshot->accum = self->a;
    snapshot->xindex = self->x;
    snapshot->yindex = self->y;
    snapshot->status = get_p(self);
}
