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
    if (addr <= CpuRamMaxAddr) {
        *data = self->ram[addr & CpuRamAddrMask];
        return true;
    }
    if (CpuCartMinAddr <= addr && addr <= CpuCartMaxAddr) {
        *data = self->cart[addr & CpuCartAddrMask];
        return true;
    }
    return false;
}

static void reset_pc(struct mos6502 *self)
{
    // TODO: what to do if read returns false?
    uint8_t lo, hi;
    read(self, ResetVector, &lo);
    read(self, ResetVector + 1, &hi);
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

static void set_p(struct mos6502 *self, uint8_t p)
{
    self->p.c = p & 0x1;
    self->p.z = p & 0x2;
    self->p.i = p & 0x4;
    self->p.d = p & 0x8;
    self->p.b = p & 0x10;
    self->p.u = p & 0x20;
    self->p.v = p & 0x40;
    self->p.n = p & 0x80;
}

//
// Public Interface
//

void cpu_powerup(struct mos6502 *self)
{
    assert(self != NULL);

    // set B, I, and unused flags high
    set_p(self, 0x34);
    // zero out other internal registers
    self->a = self->x = self->y = self->s = 0;

    // Interrupts are inverted, high means no interrupt; rw high is read
    self->signal.irq = self->signal.nmi = self->signal.res = self->signal.rw
        = true;
    self->signal.sync = false;

    // All other cpu elements are indeterminate values on powerup
}

void cpu_reset(struct mos6502 *self)
{
    // this will eventually set a signal and execute an instruction sequence
    assert(self != NULL);

    reset_pc(self);
    // mask interrupt high
    self->p.i = true;
    // NOTE: Reset runs through same sequence as BRK/IRQ
    // so the cpu does 3 phantom stack pushes;
    // on powerup this would result in $00 - $3 = $FD.
    self->s -= 3;
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
