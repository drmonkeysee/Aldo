//
//  cpu.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#include "cpu.h"

#include "bytes.h"
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
    self->pc = bytowr(lo, hi);
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

    // NOTE: set B, I, and unused flags high
    set_p(self, 0x34);
    self->a = self->x = self->y = self->s = 0;

    // NOTE: Interrupts are inverted, high means no interrupt; rw high is read
    self->signal.irq = self->signal.nmi = self->signal.res = self->signal.rw
        = true;
    self->signal.rdy = self->signal.sync = false;

    // NOTE: all other cpu elements are indeterminate on powerup
}

void cpu_reset(struct mos6502 *self)
{
    // TODO: this will eventually set a signal
    // and execute an instruction sequence
    assert(self != NULL);

    // TODO: reset sequence begins after reset signal goes from low to high
    self->signal.res = true;

    reset_pc(self);
    self->p.i = true;
    // NOTE: Reset runs through same sequence as BRK/IRQ
    // so the cpu does 3 phantom stack pushes;
    // on powerup this would result in $00 - $3 = $FD.
    self->s -= 3;

    // TODO: fake the execution of the RES sequence and load relevant
    // data into the datapath.
    self->t = 0;
    self->signal.sync = true;
    self->addrbus = self->pc;
    read(self, self->pc, &self->databus);
    self->opc = self->databus;
}

void cpu_snapshot(const struct mos6502 *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->cpu.program_counter = self->pc;
    snapshot->cpu.accumulator = self->a;
    snapshot->cpu.stack_pointer = self->s;
    snapshot->cpu.status = get_p(self);
    snapshot->cpu.xindex = self->x;
    snapshot->cpu.yindex = self->y;

    snapshot->cpu.addressbus = self->addrbus;
    snapshot->cpu.operand = bytowr(self->opl, self->oph);
    snapshot->cpu.databus = self->databus;
    snapshot->cpu.opcode = self->opc;
    snapshot->cpu.sequence_cycle = self->t;

    snapshot->lines.irq = self->signal.irq;
    snapshot->lines.nmi = self->signal.nmi;
    snapshot->lines.readwrite = self->signal.rw;
    snapshot->lines.ready = self->signal.rdy;
    snapshot->lines.reset = self->signal.res;
    snapshot->lines.sync = self->signal.sync;
}
